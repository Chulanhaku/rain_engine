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
        // Contact friction, separate from free-flight linear damping and PBR roughness.
        // Zero defaults preserve existing scenes. Both surfaces must opt in.
        // Negative/nonfinite values act as zero; effective dynamic <= static.
        // Pair coefficients use the geometric mean of the two surfaces.
        f32 static_friction = 0.0f;
        f32 dynamic_friction = 0.0f;
    };
}