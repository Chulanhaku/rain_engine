#pragma once

#include<rain/core/math/vec3.hpp>
#include<rain/core/types.hpp>
#include<rain/runtime/entity.hpp>

#include<vector>

namespace rain {
	struct physics_aabb_3d {
		vec3 minimum{};
		vec3 maximum{};
	};

	struct broadphase_proxy_3d {
		entity_id entity;

		physics_aabb_3d bounds;

		u32 layer = 0;
		u32 mask = 0;

		bool dynamic = false;
		bool trigger = false;
	};

	struct broadphase_pair_3d {
		entity_id first;
		entity_id second;
	};

	class physics_broadphase_3d {
	public:
		void clear();

		void add(const broadphase_proxy_3d& proxy);

		void build_pairs();

		[[nodiscard]] const std::vector<broadphase_pair_3d>& pairs()const;
		[[nodiscard]] usize proxy_count()const;
		[[nodiscard]] usize pair_count()const;

	private:
		std::vector<broadphase_proxy_3d>proxies_;
		std::vector<broadphase_pair_3d>pairs_;
	};
}