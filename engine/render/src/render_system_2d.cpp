#include <rain/render/render_system_2d.hpp>

#include <rain/render/sprite_2d_component.hpp>
#include <rain/runtime/transform_2d_component.hpp>

namespace rain {
    render_system_2d::render_system_2d(
        render_backend& backend,
        material_2d_registry& materials,
        u32 max_quads,
        u32 max_commands
    )
        : sprite_renderer_(backend, max_quads)
        , materials_(&materials)
        , queue_(max_commands)
        , entity_query_(make_entity_query()) {
    }

    void render_system_2d::prepare(
        world& target_world,
        const entity_query_desc& query
    ) {
        queue_.begin_frame();

        const entity_query_result entities = target_world.query_entities(query);
        for (entity_id entity : entities) {
            const transform_2d_component& transform =
                target_world.get_component<transform_2d_component>(entity);
            const sprite_2d_component& sprite =
                target_world.get_component<sprite_2d_component>(entity);

            material_2d_handle material_handle = sprite.material;
            if (!materials_->is_valid(material_handle)) {
                material_handle = materials_->default_material();
            }

            queue_.push(render_command_2d{
                .source_entity = entity,
                .center = simd_vec2(transform.position),
                .size = simd_vec2(
                    sprite.size.x * transform.scale.x,
                    sprite.size.y * transform.scale.y
                ),
                .material = material_handle,
                .tint = sprite.tint,
                .uv = sprite.uv,
                .layer = sprite.layer,
                .order_in_layer = sprite.order_in_layer
            });
        }

        queue_.sort(*materials_);
        last_command_count_ = static_cast<u32>(queue_.size());
    }

    void render_system_2d::submit(const camera_2d& camera) {
        sprite_renderer_.begin(camera);

        for (const render_command_2d& command : queue_.commands()) {
            const material_2d* material = materials_->try_get(command.material);
            if (material == nullptr) {
                material = materials_->try_get(materials_->default_material());
            }
            if (material == nullptr) {
                continue;
            }

            sprite_renderer_.draw_rect_world(
                sprite_rect_world{
                    .center = command.center,
                    .size = command.size
                },
                command.tint,
                material->texture,
                command.uv,
                material->blend_mode
            );
        }

        sprite_renderer_.end();
        last_quad_count_ = sprite_renderer_.quad_count();
    }

    void render_system_2d::clear() {
        queue_.begin_frame();
        last_quad_count_ = 0;
        last_command_count_ = 0;
    }

    entity_query_desc render_system_2d::make_entity_query() {
        tag_query tags;
        tags.require_all(tag_id{"object.renderable"});
        tags.reject(tag_id{"render.hidden"});

        return entity_query_desc{
            .required_components = {
                get_type_id<transform_2d_component>(),
                get_type_id<sprite_2d_component>()
            },
            .required_tags = tags,
            .require_alive = true,
            .require_active = true
        };
    }

    const render_queue_2d& render_system_2d::queue() const {
        return queue_;
    }

    u32 render_system_2d::last_quad_count() const {
        return last_quad_count_;
    }

    u32 render_system_2d::last_command_count() const {
        return last_command_count_;
    }

    void render_system_2d::render(world& target_world, const camera_2d& camera) {
        prepare(target_world, entity_query_);
        submit(camera);
    }

    void render_system_2d::set_entity_query(const entity_query_desc& desc) {
        entity_query_ = desc;
    }

    const entity_query_desc& render_system_2d::entity_query() const {
        return entity_query_;
    }

    void render_prepare_system_2d(system_context& context, void* user_data) {
        if (context.target_world == nullptr ||
            context.entity_query == nullptr ||
            user_data == nullptr) {
            return;
        }

        auto* render_system = static_cast<render_system_2d*>(user_data);
        render_system->prepare(*context.target_world, *context.entity_query);
    }
}
