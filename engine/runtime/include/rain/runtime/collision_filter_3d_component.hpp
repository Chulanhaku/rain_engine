#pragma once

#include <rain/core/types.hpp>

namespace rain {
namespace collision_layer_3d {
inline constexpr u32 default_layer = 1u << 0;
inline constexpr u32 world = 1u << 1;
inline constexpr u32 player = 1u << 2;
inline constexpr u32 enemy = 1u << 3;
inline constexpr u32 projectile = 1u << 4;
inline constexpr u32 trigger = 1u << 5;
inline constexpr u32 all = ~u32{0};
}

struct collision_filter_3d_component {
    u32 layer = collision_layer_3d::default_layer;
    u32 mask = collision_layer_3d::all;
};

[[nodiscard]] inline bool collision_filters_match(
    const collision_filter_3d_component& first,
    const collision_filter_3d_component& second) noexcept {
    return (first.mask & second.layer) != 0 && (second.mask & first.layer) != 0;
}
}
