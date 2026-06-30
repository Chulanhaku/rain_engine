#include<rain/runtime/movement_system_2d.hpp>

#include<rain/runtime/transform_2d_component.hpp>
#include<rain/runtime/velocity_2d_component.hpp>
#include<rain/runtime/world.hpp>
#include<rain/core/tag/tag_query.hpp>

namespace rain {
	namespace {
		const tag_query& movement_required_query() {
			static const tag_query query = [] {
				tag_query result;
				result.require_all(tag_id{ "object.movable" });
				result.reject(tag_id{ "state.frozen" });
				result.reject(tag_id{ "state.rooted" });
				return result;
			}();

			return query;
		}

		[[nodiscard]] f32 movement_speed_scale(const world& target_world, entity_id entity) {
			f32 scale = 1.0f;

			if (target_world.has_tag(entity, tag_id{ "movement.mode.slow" }))scale *= 0.5f;
			if (target_world.has_tag(entity, tag_id{ "movement.mode.fast" }))scale *= 1.5f;

			return scale;
		}
	}

	void movement_system_2d(system_context& context, void* user_data) {
		(void)user_data;

		if (context.target_world == nullptr) {
			return;
		}

		world& target_world = *context.target_world;

		auto* velocity_pool = target_world.try_get_component_pool<velocity_2d_component>();
		auto* transform_pool = target_world.try_get_component_pool<transform_2d_component>();

		if (velocity_pool == nullptr || transform_pool == nullptr)return;

		const auto& entities = velocity_pool->entities();
		const auto& velocities = velocity_pool->values();
		const tag_query& query = movement_required_query();

		for (usize i = 0; i < velocity_pool->size(); ++i) {
			const entity_id entity = entities[i];

			if (!target_world.is_entity_active(entity))continue;
			if (!target_world.matches_tags(entity, query))continue;
			if (!transform_pool->has(entity))continue;

			const velocity_2d_component& velocity = velocities[i];
			transform_2d_component& transform = transform_pool->get(entity);
			const f32 speed_scale = movement_speed_scale(target_world, entity);

			transform.position.x += velocity.x * context.delta_seconds * speed_scale;
			transform.position.y += velocity.y * context.delta_seconds * speed_scale;
		}
	}
}