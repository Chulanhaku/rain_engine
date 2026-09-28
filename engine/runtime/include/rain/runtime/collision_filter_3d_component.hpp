#pragma once

#include<rain/core/types.hpp>

namespace rain {
	namespace collision_layer_3d {
		constexpr u32 default_layer = 1u << 0;

		constexpr u32 world = 1u << 1;

		constexpr u32 player = 1u << 2;

		constexpr u32 enemy = 1u << 3;
		constexpr u32 projectile = 1u << 4;
		constexpr u32 trigger = 1u << 5;
		constexpr u32 all = 1u << 0xffffffffu;
	}

	struct collision_filter_3d_component {
		u32 layer = collision_layer_3d::default_layer;

		u32 mask = collision_layer_3d::all;
	};

	[[nodiscard]] inline bool collision_filters_match(const collision_filter_3d_component&lhs, collision_filter_3d_component&rhs) {
		const bool lhs_accepts_rhs = (lhs.mask & rhs.mask) != 0;

		const bool rhs_accepts_lhs = (rhs.mask & lhs.mask) != 0;

		return lhs_accepts_rhs && rhs_accepts_lhs;
	}


}