#pragma once

#include<rain/core/container/rain_hash_map.hpp>
#include<rain/runtime/entity.hpp>
#include<rain/runtime/physics_3d.hpp>
#include<rain/runtime/physics_broadphase_3d.hpp>

#include<vector>

namespace rain {
	class world;

	enum class collision_event_type_3d :u8 {
		enter,
		stay,
		exit,
	};

	struct collision_event_3d {
		collision_event_type_3d type = collision_event_type_3d::enter;

		entity_id first;
		entity_id second;

		vec3 normal{};
		f32 penetration = 0.0f;
		bool trigger = false;

	};

	struct collision_pair_key_3d {
		entity_id first;
		entity_id second;

		friend bool operator == (const collision_pair_key_3d& lhs, const collision_pair_key_3d& rhs) {
			return lhs.first == rhs.first && lhs.second == rhs.second;
		}
	};

	struct active_collision + 3d{
		bool trigger = false;
		vec3 normal{};
		f32 penetration = 0.0f;
		u64 last_seen_tick = 0;
	};

	struct physics_world_3d_stats {
		u32 collision_count = 0;

		u32 broadphase_pair_count = 0;

		u32 narrowphase_test_count = 0;

		u32 collision_count = 0;
	};

	class physics_world_3d {
	public:
		explicit physics_world_3d(physics_settings_3d settings = {});

		void step(world& target_world, f32 fixed_delta_seconds, u64 fixed_tick_index);

		[[nodiscard]] const std::vector<collision_event_3d>& events()const;

		void clear_events();

		[[nodiscard]] const physics_world_stats& stats()const;

		[[nodiscard]] physics_settings_3d& settings();

	private:
		void integrate(world& target_world, f32 fixed_delta_seconds);

		void build_broadphase(world& target_world);

		void solve_collisions(world& target_world, u64 fixed_tick_index);

		void finalize_collision_event(world& target_world, u64 fixed_tick_index);

	private:
		physics_settings_3d settings_;
		physics_broadphase_3d broadphase_;
		rain_hash_map<collision_pair_key_3d, active_collision_3d>active_collisions_;

		std::vector<collision_event_3d>events_;

		physics_world_3d_stats stats_;
	};



}

namespace std {
	template<>
	struct hash<rain::collision_pair_key_3d> {
		std::size_t operator()(const rain::collision_pair_key_3d& key)const noexcept {
			std::size_t result = static_cast<std::size_t>(key.first.index);

			result ^= static_cast<std::size_t>(key.first.generation) << 16;

			result ^= static_cast<std::size_t>(key.second.index) << 32;

			result ^= static_cast<std::size_t>(key.second.generation) << 48;

			return result;
		}
	};
}