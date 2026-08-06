#include <rain/runtime/rotation_system_3d.hpp>

#include<rain/runtime/angular_velocity_3d_component.hpp>

#include<rain/runtime/transform_3d_component.hpp>
#include<rain/runtime/world.hpp>

namespace rain {
	void rotation_system_3d(system_context& context, void* user_data) {
		(void)user_data;

		if (context.target_world == nullptr || context.entity_query == nullptr) {
			return;
		}

		world& target_world = *context.target_world;

		const entity_query_result entities = target_world.query_entities(*context.entity_query);

		for (entity_id entity : entities) {
			transform_3d_component& transform = target_world.get_component<tranform_3d_component>(entity);

			const angular_velocity_3d_component& velocity = target_world.get_component<angular_velocity_3d_component>(entity);

			transform.rotation.x += velocity.radians_per_second.x * context.delta_seconds;

			transform.rotation.y += velocity.radians_per_second.y * context.delta_seconds;

			transform.rotation.z += velocity.radians_per_second.z * context.delta_seconds;
		}
	}
}