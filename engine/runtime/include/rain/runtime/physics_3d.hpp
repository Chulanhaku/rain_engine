#pragma once

#include <rain/core/math/vec3.hpp>
#include <rain/core/types.hpp>

namespace rain
{
    struct physics_settings_3d
    {
        vec3 gravity{0.0f, -9.81f, 0.0f};
        u32 solver_iterations = 4;
        // Suppress tiny restitution impulses so resting bodies can settle.
        f32 restitution_velocity_threshold = 0.5f;
    };
}