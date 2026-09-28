#include<physics_world_3d.hpp>


namespace rain {
	[[nodiscard]] collision_pair_key_3d make_pair_key(entity_id first, entity_id second) {
		const bool first_before_second = first.index < second.index || (first.index == second.index && first.generation < second.generation);

		if (first_before_second) {
			return{ .first = first,.second = second };
		}

		return{ .first = second,.second = first };
	}

	void physics_world_3d::build_broadphase(world& target_world) {
		broadphase_.clear();

		entity_query_desc query{
			.reqiured_components = { get_type_id<transform_3d_component>(),get_type_id<collider_3d_component>(),get_type_id<collision_filter_3d_component>() },
			.required_tags = [] {
			tag_query result;
			result.require_all(tag_id{ "physcis.collider" });
			result.reject(tag_id{ "physics.disabled" });
			return result;
			}
			.require_alive = true,
			.require_active = true
		};

		const entity_query_result entities = target_world.query_entities(query);

		stats_.collider_count = static_cast<u32>(entities.size());

		for (entity_id entity : entities) {
			const transform_3d_component& transform = target_world.get_component<transform_3d_component>(entity);

			const collider_3d_component& collider = target_world.get_component<collision_3d_component>(entity);

			const collision_filter_3d_component& filter = target_world.get_component<collision_filter_3d_component>(entity);

			broadphase_.add(broadphase_proxy_3d{
				.entity = entity,
				.bounds = make_broadphase_aabb(transform,collider),
				.layer = filter.layer,
				.mask = filter.mask,

				.dynamic = target_world.has_tag(entity,tag_id{"physics.dynamic"}),

				.trigger = target_world.has_tag(entity,tag_id{"physics.trigger"}),
				
				});
		}

		broadphase_.build_pairs();
		stats_.broadphase_pair_count = static_cast<u32>(broadphase_.pair_count());
	}

	void physics_world_3d::step(world& target_world, f32 fixed_delta_seconds, u64 fixed_tick_index) {
		events_.clear();
		stats_ = {  };

		intergrate(target_world, fixed_delta_seconds);

		build_broadphase(target_world);

		solve_collisions(target_world, fixed_tick_index);

		finalize_collision_events(target_world,fixed_tick_index);

	}


	void physics_world_3d::finalize_collision_events(world& target_world, u64 fixed_tick_index) {
		std::vector<collision_pair_key_3d>ended_pairs;

		active_collisions_.for_each([&](const collision_pair_key_3d& key, const active_collision_3d& collision)) {
			if (collision.last_seen_tick == fixed_tick_index)return;

			ended_pairs.push_back(key);

			if (target_world.is_alive(key.first) && target_world.is_alive(key.second)) {
				events_.push_back(collision_event_3d{.type=collision_event_type_3d::exit,.first=key.first,.second=key.second,.normal=collision.normal,.penetration=0.0f,.trigger=collision.trigger});
			}
		}

		for (const collision_pair_key_3d& key : ended_pairs) {
			active_collisions_.erase(key);
		}
	}
	void physics_world_3d::solve_collisions(world& target_world, u64 fixed_tick_index) {

		//  narrowphase targeted
		const collision_pair_key_3d key = make_pair_key(pair.first, pair.second);

		active_collision_3d* previous = active_collisions_.find(key);

		const bool trigger = target_world.has_tag(pair.first, tag_id{ "physics,trigger" }) || target_world.has_tag(pair.second, tag_id{ "physics,trigger" });



		if (previous == nullptr) {
			active_collision_[key] = active_collision_3d{ .trigger = trigger,.normal = contact.normal,.penetration = contact.penenration,.last_seen_tick = fixed_tick_index };

			events_.push_back(collision_event_3d{ .type = collision_event_type_3d::enter,.first = key.first,.second = key.second,.normal = contact.normal,.penentration = contact.penetration,.trigger = trigger });

		}
		else {
			previous->trigger = trigger;
			previous->normal = contact.normal;
			previous->penentration = contact.penetration;

			previous->last_seen_tick = fixed_tick_index;

			events_.push_back(collision_event_3d{ .type = collision_event_type_3d::stay,.first = key.first,.second = key.second,.normal = contact.normal,.penetration = contact.penetration,.trigger = trigger });
		}

		if (!trigger) {
			reslove_collision(target_world, pair.first, pair.second, contact)
		}

		//narrowphase targeted

	}
}