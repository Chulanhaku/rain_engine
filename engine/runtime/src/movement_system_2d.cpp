#include<rain/runtime/movement_system_2d.hpp>

#include<rain/runtime/transform_2d_component.hpp>
#include<rain/runtime/velocity_2d_component.hpp>
#include<rain/runtime/world.hpp>

namespace rain {
	void movement_system_2d(system_context& context, void* user_data) {
		(void)user_data;

		if (context.target_world == nullptr) {
			return;
		}

		world& target_world = *context.target_world;

		auto* velocity_pool = target_world.try_get_component_pool<velocity_2d_component>();

		auto* transform_pool = target_world.try_get_component_pool<transform_2d_component>();

		if (velocity_pool == nullptr || transform_pool == nullptr) return;

		const auto& entities = velocity_pool->entities();
		const auto& velocities = velocity_pool->values();

		for (usize i = 0; i < velocity_pool->size(); ++i) {
			const entity_id entity = entities[i];

			if (!transform_pool->has(entity))continue;

			const velocity_2d_component& velocity = velocities[i];

			transform_2d_component& transform = transform_pool->get(entity);

			transform.position.x += velocity.x * context.delta_seconds;
			transform.position.y += velocity.y * context.delta_seconds;
		}
	}
}
