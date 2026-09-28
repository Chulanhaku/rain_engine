#pragma once

#include <rain/core/types.hpp>

namespace rain
{
    struct rigid_body_3d_component
    {
        // Zero mass is immovable. physics.static, physics.kinematic and
        // state.frozen override physics.dynamic without changing stored mass.
        f32 inverse_mass = 1.0f;
        f32 gravity_scale = 1.0f;
        f32 linear_damping = 0.02f;
        // Bounce coefficient, clamped to [0, 1] by the collision solver.
        f32 restitution = 0.0f;
    };
}