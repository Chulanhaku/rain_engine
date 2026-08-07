#include <rain/render/render_system_3d.hpp>
#include <rain/render/mesh_3d_shader_source.hpp>

#include <algorithm>
#include <cstddef>

namespace rain {
    namespace {
        struct object_constants_3d {
            mat4 world_matrix;
            mat4 world_view_projection;
        };

        struct scene_constants_3d {
            vec4 light_direction;
            vec4 light_color;
            vec4 ambient_color;
            vec4 camera_position;
        };

        struct material_constants_3d {
            vec4 base_color;
            vec4 material_options;
        };

        static_assert(sizeof(object_constants_3d) == 128);
        static_assert(sizeof(scene_constants_3d) == 64);
        static_assert(sizeof(material_constants_3d) == 32);

        camera_3d_frame build_camera_frame(const transform_3d_component& transform, const camera_3d_component& camera, f32 aspect_ratio) {
            camera_3d_frame result{};
            const vec3 forward = forward_from_euler(transform.rotation);
            result.position = transform.position;
            result.view = make_look_at_lh(transform.position, transform.position + forward, vec3{0.0f, 1.0f, 0.0f});
            result.projection = make_perspective_fov_lh(camera.vertical_fov_radians, aspect_ratio, camera.near_plane, camera.far_plane);
            result.view_projection = result.view * result.projection;
            return result;
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
        pipeline_ = backend_->create_pipeline_state({
            .name = "pipeline.mesh_3d", .shader = shader_,
            .vertex_attributes = {
                {"POSITION", 0, vertex_attribute_format::r32g32b32_float, 0, static_cast<u32>(offsetof(mesh_vertex_3d, position))},
                {"NORMAL", 0, vertex_attribute_format::r32g32b32_float, 0, static_cast<u32>(offsetof(mesh_vertex_3d, normal))},
                {"TEXCOORD", 0, vertex_attribute_format::r32g32_float, 0, static_cast<u32>(offsetof(mesh_vertex_3d, uv))}
            },
            .topology = primitive_topology::triangle_list, .blend_mode = render_blend_mode::opaque,
            .cull_mode = render_cull_mode::back, .front_counter_clockwise = false,
            .depth_test_enabled = true, .depth_write_enabled = true
        });
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
        backend_->destroy_pipeline_state(pipeline_);
        backend_->destroy_shader_program(shader_);
    }

    entity_query_desc render_system_3d::make_render_query() {
        tag_query tags; tags.require_all(tag_id{"object.renderable"}); tags.require_all(tag_id{"render.3d"}); tags.reject(tag_id{"render.hidden"});
        return {.required_components = {get_type_id<transform_3d_component>(), get_type_id<mesh_3d_component>()}, .required_tags = tags, .require_alive = true, .require_active = true};
    }

    entity_query_desc render_system_3d::make_camera_query() {
        tag_query tags; tags.require_all(tag_id{"camera.3d"}); tags.require_all(tag_id{"camera.active"});
        return {.required_components = {get_type_id<transform_3d_component>(), get_type_id<camera_3d_component>()}, .required_tags = tags, .require_alive = true, .require_active = true};
    }

    entity_query_desc render_system_3d::make_light_query() {
        tag_query tags; tags.require_all(tag_id{"light.directional"}); tags.require_all(tag_id{"light.active"});
        return {.required_components = {get_type_id<directional_light_3d_component>()}, .required_tags = tags, .require_alive = true, .require_active = true};
    }

    void render_system_3d::prepare(world& target_world) {
        commands_.clear();
        const entity_query_result cameras = target_world.query_entities(camera_query_);
        if (cameras.empty() || backend_->height() == 0) { has_camera_ = false; return; }
        const entity_id camera_entity = cameras.entities.front();
        const auto& camera_transform = target_world.get_component<transform_3d_component>(camera_entity);
        const auto& camera = target_world.get_component<camera_3d_component>(camera_entity);
        const f32 aspect_ratio = static_cast<f32>(backend_->width()) / static_cast<f32>(backend_->height());
        camera_frame_ = build_camera_frame(camera_transform, camera, aspect_ratio);
        has_camera_ = true;
        const entity_query_result lights = target_world.query_entities(light_query_);
        light_ = lights.empty() ? directional_light_3d_component{} : target_world.get_component<directional_light_3d_component>(lights.entities.front());
        u64 submission_index = 0;
        for (entity_id entity : target_world.query_entities(render_query_)) {
            const auto& transform = target_world.get_component<transform_3d_component>(entity);
            const auto& mesh = target_world.get_component<mesh_3d_component>(entity);
            const vec3 difference = transform.position - camera_frame_.position;
            commands_.push_back({.source_entity = entity, .world_matrix = transform.matrix(), .mesh = mesh.mesh, .material = mesh.material, .tint = mesh.tint, .camera_distance_squared = length_squared(difference), .layer = mesh.layer, .submission_index = submission_index++});
        }
        std::stable_sort(commands_.begin(), commands_.end(), [](const render_command_3d& lhs, const render_command_3d& rhs) {
            if (lhs.layer != rhs.layer) return lhs.layer < rhs.layer;
            return lhs.camera_distance_squared < rhs.camera_distance_squared;
        });
    }

    void render_system_3d::submit() {
        if (!has_camera_) return;
        backend_->set_pipeline_state(pipeline_);
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
            const bool has_texture = backend_->is_valid(material->albedo_texture);
            const object_constants_3d object{.world_matrix = command.world_matrix, .world_view_projection = command.world_matrix * camera_frame_.view_projection};
            const material_constants_3d material_data{.base_color = multiply_color(material->base_color, command.tint), .material_options = {has_texture ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f}};
            
            backend_->update_buffer(object_constants_, &object, sizeof(object));
            backend_->update_buffer(material_constants_, &material_data, sizeof(material_data));
            backend_->set_vertex_constant_buffer(object_constants_, 0);
            backend_->set_pixel_constant_buffer(material_constants_, 2);
            backend_->set_texture_2d(material->albedo_texture, 0);
            backend_->set_vertex_buffer(mesh->vertex_buffer, 0);
            backend_->set_index_buffer(mesh->index_buffer, render_index_format::uint32);
            backend_->draw_indexed(mesh->index_count, 0, 0);
        }
    }

    void render_system_3d::clear() { commands_.clear(); has_camera_ = false; }
    usize render_system_3d::command_count() const { return commands_.size(); }

    void render_prepare_system_3d(system_context& context, void* user_data) {
        if (context.target_world == nullptr || user_data == nullptr) return;
        static_cast<render_system_3d*>(user_data)->prepare(*context.target_world);
    }
}
