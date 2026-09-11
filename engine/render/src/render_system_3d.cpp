#include <rain/render/render_system_3d.hpp>
#include <rain/render/mesh_3d_shader_source.hpp>
#include <rain/render/frustum_culling_3d.hpp>
#include <rain/runtime/world_transform_3d_component.hpp>

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace rain {
    namespace {
        struct alignas(16) object_constants_3d {
            mat4 world_matrix;
            mat4 world_view_projection;
            mat4 normal_matrix;
        };

        struct alignas(16) scene_constants_3d {
            vec4 light_direction;
            vec4 light_color;
            vec4 ambient_color;
            vec4 camera_position;
        };

        struct alignas(16) material_constants_3d {
            vec4 base_color;
            vec4 material_parameters;
            vec4 material_options;
        };

        static_assert(sizeof(object_constants_3d) == 192);
        static_assert(sizeof(scene_constants_3d) == 64);
        static_assert(sizeof(material_constants_3d) == 48);

        camera_3d_frame build_camera_frame(const world_transform_3d_component& transform, const camera_3d_component& camera, f32 aspect_ratio) {
            camera_3d_frame result{};
            const vec3 forward = transform.forward;

            result.vertical_fov_radians = camera.vertical_fov_radians;
            result.aspect_ratio = aspect_ratio;
            result.near_plane = camera.near_plane;
            result.far_plane = camera.far_plane;
            result.position = transform.position;
            result.view = make_look_at_lh(transform.position, transform.position + forward, transform.up);
            result.projection = make_perspective_fov_lh(result.vertical_fov_radians, aspect_ratio, result.near_plane, result.far_plane);
            result.view_projection = result.view * result.projection;
            return result;
        }

        f32 unit_factor(f32 value, f32 fallback) {
            return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : fallback;
        }

        usize blend_index(render_blend_mode mode) {
            if (mode == render_blend_mode::alpha) return 1;
            if (mode == render_blend_mode::additive) return 2;
            return 0;
        }

        bool has_mirrored_winding(const mat4& matrix) {
            const auto& m = matrix.values;
            return dot(cross(vec3{m[0][0], m[0][1], m[0][2]},
                             vec3{m[1][0], m[1][1], m[1][2]}),
                             vec3{m[2][0], m[2][1], m[2][2]}) < 0.0f;
        }

        vec4 multiply_color(vec4 lhs, vec4 rhs) {
            return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
        }
    }

    render_system_3d::render_system_3d(render_backend& backend, mesh_3d_registry& meshes, material_3d_registry& materials)
        : backend_(&backend), meshes_(&meshes), materials_(&materials),
          camera_query_(make_camera_query()), render_query_(make_render_query()), light_query_(make_light_query()) {
        shader_ = backend_->create_shader_program({
            .name = "shader.mesh_3d",
            .vertex_source = detail::mesh_3d_shader_source,
            .vertex_entry = "vertex_main",
            .pixel_source = detail::mesh_3d_shader_source,
            .pixel_entry = "pixel_main"
        });
        for (usize blend = 0; blend < 3; ++blend)
        for (usize double_sided = 0; double_sided < 2; ++double_sided)
        for (usize mirrored = 0; mirrored < 2; ++mirrored) {
            pipelines_[blend][double_sided][mirrored] = backend_->create_pipeline_state({
                .name = "pipeline.mesh_3d." + std::to_string(blend) + "." + std::to_string(double_sided) + "." + std::to_string(mirrored), .shader = shader_,
                .vertex_attributes = {
                    {"POSITION", 0, vertex_attribute_format::r32g32b32_float, 0, static_cast<u32>(offsetof(mesh_vertex_3d, position))},
                    {"NORMAL", 0, vertex_attribute_format::r32g32b32_float, 0, static_cast<u32>(offsetof(mesh_vertex_3d, normal))},
                    {"TEXCOORD", 0, vertex_attribute_format::r32g32_float, 0, static_cast<u32>(offsetof(mesh_vertex_3d, uv))}
                },
                .topology = primitive_topology::triangle_list, .blend_mode = static_cast<render_blend_mode>(blend),
                .cull_mode = double_sided ? render_cull_mode::none : render_cull_mode::back, .front_counter_clockwise = mirrored != 0,
                .depth_test_enabled = true, .depth_write_enabled = blend == 0, .srgb_write_enabled = true
            });
        }
        object_constants_ = backend_->create_constant_buffer({
            .name = "buffer.mesh_3d.object_constants", .bind = render_buffer_bind::constant_buffer,
            .usage = render_buffer_usage::dynamic, .size_bytes = sizeof(object_constants_3d), .stride_bytes = sizeof(vec4)
        });
        scene_constants_ = backend_->create_constant_buffer({
            .name = "buffer.mesh_3d.scene_constants", .bind = render_buffer_bind::constant_buffer,
            .usage = render_buffer_usage::dynamic, .size_bytes = sizeof(scene_constants_3d), .stride_bytes = sizeof(vec4)
        });
        material_constants_ = backend_->create_constant_buffer({
            .name = "buffer.mesh_3d.material_constants", .bind = render_buffer_bind::constant_buffer,
            .usage = render_buffer_usage::dynamic, .size_bytes = sizeof(material_constants_3d), .stride_bytes = sizeof(vec4)
        });
    }

    render_system_3d::~render_system_3d() {
        if (backend_ == nullptr) return;
        backend_->destroy_render_buffer(material_constants_);
        backend_->destroy_render_buffer(scene_constants_);
        backend_->destroy_render_buffer(object_constants_);
        for (auto& blend : pipelines_)
            for (auto& side : blend)
                for (auto pipeline : side) backend_->destroy_pipeline_state(pipeline);
        backend_->destroy_shader_program(shader_);
    }

    entity_query_desc render_system_3d::make_render_query() {
        tag_query tags; tags.require_all(tag_id{"object.renderable"}); tags.require_all(tag_id{"render.3d"}); tags.reject(tag_id{"render.hidden"});
        return {.required_components = {get_type_id<world_transform_3d_component>(), get_type_id<mesh_3d_component>()}, .required_tags = tags, .require_alive = true, .require_active = true};
    }

    entity_query_desc render_system_3d::make_camera_query() {
        tag_query tags; tags.require_all(tag_id{"camera.3d"}); tags.require_all(tag_id{"camera.active"});
        return {.required_components = {get_type_id<world_transform_3d_component>(), get_type_id<camera_3d_component>()}, .required_tags = tags, .require_alive = true, .require_active = true};
    }

    entity_query_desc render_system_3d::make_light_query() {
        tag_query tags; tags.require_all(tag_id{"light.directional"}); tags.require_all(tag_id{"light.active"});
        return {.required_components = {get_type_id<directional_light_3d_component>()}, .required_tags = tags, .require_alive = true, .require_active = true};
    }

    void render_system_3d::prepare(world& target_world) {
        clear();
        const entity_query_result cameras = target_world.query_entities(camera_query_);
        if (cameras.empty() || backend_->height() == 0 || backend_->width() == 0) { has_camera_ = false; return; }
        const entity_id camera_entity = cameras.entities.front();
        const auto& camera_transform = target_world.get_component<world_transform_3d_component>(camera_entity);
        const auto& camera = target_world.get_component<camera_3d_component>(camera_entity);
        const f32 aspect_ratio = static_cast<f32>(backend_->width()) / static_cast<f32>(backend_->height());
        if (!std::isfinite(camera.vertical_fov_radians) || camera.vertical_fov_radians <= 0.0f ||
            camera.vertical_fov_radians >= 3.14159265f || !std::isfinite(camera.near_plane) ||
            !std::isfinite(camera.far_plane) || camera.near_plane <= 0.0f ||
            camera.far_plane <= camera.near_plane) return;
        camera_frame_ = build_camera_frame(camera_transform, camera, aspect_ratio);
        has_camera_ = true;
        const entity_query_result lights = target_world.query_entities(light_query_);
        light_ = lights.empty() ? directional_light_3d_component{} : target_world.get_component<directional_light_3d_component>(lights.entities.front());
        const camera_frustum_3d frustum = extract_camera_frustum(camera_frame_.view_projection);
        u64 submission_index = 0;
        for (entity_id entity : target_world.query_entities(render_query_)) {
            if (target_world.has_tag_in_hierarchy(entity, tag_id{"render.hidden"})) continue;
            const auto& transform = target_world.get_component<world_transform_3d_component>(entity);
            const auto& component = target_world.get_component<mesh_3d_component>(entity);
            const mesh_3d* mesh = meshes_->try_get(component.mesh);
            if (mesh == nullptr) continue;
            ++stats_.candidate_count;

            const bool use_frustum_culling =
                target_world.has_tag_in_hierarchy(entity, tag_id{"render.frustum_cull"});
            const bool always_visible =
                target_world.has_tag_in_hierarchy(entity, tag_id{"render.always_visible"});
            const bounding_sphere_3d world_bounds = transform_bounding_sphere(mesh->local_bounds, transform.matrix);
            if (use_frustum_culling && !always_visible &&
                !sphere_inside_camera_frustum(world_bounds, frustum)) {
                ++stats_.culled_count;
                continue;
            }

            const material_3d* material = materials_->try_get(component.material);
            if (material == nullptr) material = materials_->try_get(materials_->default_material());
            const vec3 difference = world_bounds.center - camera_frame_.position;
            const f32 view_depth = transform_point(world_bounds.center, camera_frame_.view).z;
            ++stats_.visible_count;
            commands_.push_back({
                .source_entity = entity, .world_matrix = transform.matrix,
                .mesh = component.mesh, .material = component.material, .tint = component.tint,
                .camera_distance_squared = length_squared(difference),
                .layer = component.layer, .submission_index = submission_index++,
                .camera_view_depth = std::isfinite(view_depth) ? view_depth : 0.0f,
                .blend_mode = material ? material->blend_mode : render_blend_mode::opaque
            });
        }

        std::stable_sort(commands_.begin(), commands_.end(), [](const render_command_3d& lhs, const render_command_3d& rhs) {
            if (lhs.layer != rhs.layer) return lhs.layer < rhs.layer;
            const bool lhs_transparent = lhs.blend_mode != render_blend_mode::opaque;
            const bool rhs_transparent = rhs.blend_mode != render_blend_mode::opaque;
            if (lhs_transparent != rhs_transparent) return !lhs_transparent;
            return lhs_transparent ? lhs.camera_view_depth > rhs.camera_view_depth
                                   : lhs.camera_view_depth < rhs.camera_view_depth;
        });
    }

    void render_system_3d::submit() {
        if (!has_camera_) return;
        const scene_constants_3d scene{
            .light_direction = {light_.direction.x, light_.direction.y, light_.direction.z, 0.0f},
            .light_color = {light_.color.x * light_.intensity, light_.color.y * light_.intensity, light_.color.z * light_.intensity, 1.0f},
            .ambient_color = {light_.ambient_color.x, light_.ambient_color.y, light_.ambient_color.z, 1.0f},
            .camera_position = {camera_frame_.position.x, camera_frame_.position.y, camera_frame_.position.z, 1.0f}
        };
        backend_->update_buffer(scene_constants_, &scene, sizeof(scene));
        backend_->set_vertex_constant_buffer(scene_constants_, 1);
        backend_->set_pixel_constant_buffer(scene_constants_, 1);

        for (const render_command_3d& command : commands_) {
            const mesh_3d* mesh = meshes_->try_get(command.mesh);
            if (mesh == nullptr) continue;
            const material_3d* material = materials_->try_get(command.material);
            if (material == nullptr) material = materials_->try_get(materials_->default_material());
            if (material == nullptr) continue;
            backend_->set_pipeline_state(pipelines_[blend_index(command.blend_mode)]
                [material->double_sided ? 1 : 0][has_mirrored_winding(command.world_matrix) ? 1 : 0]);
            const bool has_texture = backend_->is_valid(material->albedo_texture);
            const object_constants_3d object{.world_matrix = command.world_matrix, .world_view_projection = command.world_matrix * camera_frame_.view_projection,.normal_matrix = make_normal_matrix(command.world_matrix)};
            const f32 cutoff = std::isfinite(material->alpha_cutoff) && material->alpha_cutoff >= 0.0f
                ? std::clamp(material->alpha_cutoff, 0.0f, 1.0f) : -1.0f;
            const material_constants_3d material_data{
                .base_color = multiply_color(material->base_color, command.tint),
                .material_parameters = {unit_factor(material->metallic_factor, 0.0f),
                    unit_factor(material->roughness_factor, 1.0f), cutoff,
                    material->albedo_manual_srgb_decode ? 1.0f : 0.0f},
                .material_options = {has_texture ? 1.0f : 0.0f,
                    backend_->is_valid(material->metallic_roughness_texture) ? 1.0f : 0.0f,
                    command.blend_mode != render_blend_mode::opaque ? 1.0f : 0.0f,
                    material->double_sided ? 1.0f : 0.0f}
            };

            backend_->update_buffer(object_constants_, &object, sizeof(object));
            backend_->update_buffer(material_constants_, &material_data, sizeof(material_data));
            backend_->set_vertex_constant_buffer(object_constants_, 0);
            backend_->set_pixel_constant_buffer(material_constants_, 2);
            backend_->set_texture_2d(material->albedo_texture, 0);
            backend_->set_texture_2d(material->metallic_roughness_texture, 1);
            backend_->set_vertex_buffer(mesh->vertex_buffer, 0);
            backend_->set_index_buffer(mesh->index_buffer, render_index_format::uint32);
            backend_->draw_indexed(mesh->index_count, 0, 0);
        }
    }

    void render_system_3d::clear() { commands_.clear(); has_camera_ = false; stats_ = {}; }
    usize render_system_3d::command_count() const { return commands_.size(); }

    void render_prepare_system_3d(system_context& context, void* user_data) {
        if (context.target_world == nullptr || user_data == nullptr) return;
        static_cast<render_system_3d*>(user_data)->prepare(*context.target_world);
    }
}
