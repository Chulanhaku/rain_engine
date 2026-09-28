#pragma once

#include <rain/core/math/vec3.hpp>
#include <rain/core/types.hpp>

namespace rain
{
    enum class collider_shape_3d : u8
    {
        box,
        sphere
    };

    struct collider_3d_component
    {
        collider_shape_3d shape = collider_shape_3d::box;
        // Local-space dimensions and offset. V0 uses world AABBs for boxes;
        // rotated boxes are conservative bounds, not oriented-box contacts.
        vec3 half_extents{0.5f, 0.5f, 0.5f};
        f32 radius = 0.5f;
        vec3 center{};
    };
}