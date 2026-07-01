#include<rain/runtime/movement_system_2d.hpp>

#include<rain/runtime/transform_2d_component.hpp>
#include<rain/runtime/velocity_2d_component.hpp>
#include<rain/runtime/world.hpp>

namespace rain {
	namespace {
		[[nodiscard]] f32 movement_speed_scale(const world& target_world, entity_id entity) {
			f32 scale = 1.0f;

			if (target_world.has_tag(entity, tag_id{ "movement.mode.slow" }))scale *= 0.5f;
			if (target_world.has_tag(entity, tag_id{ "movement.mode.fast" }))scale *= 1.5f;

			return scale;
		}
	}

    void movement_system_2d(system_context& context, void* user_data)
    {
        (void)user_data;

        if (context.target_world == nullptr || context.entity_query == nullptr)
        {
            return;
        }

        world& target_world = *context.target_world;

        const entity_query_result entities =
            target_world.query_entities(*context.entity_query);

        for (entity_id entity : entities)
        {
            transform_2d_component& transform =
                target_world.get_component<transform_2d_component>(entity);

            const velocity_2d_component& velocity =
                target_world.get_component<velocity_2d_component>(entity);

            const f32 speed_scale = movement_speed_scale(target_world, entity);

            transform.position.x += velocity.x * speed_scale * context.delta_seconds;
            transform.position.y += velocity.y * speed_scale * context.delta_seconds;
        }
    }
}