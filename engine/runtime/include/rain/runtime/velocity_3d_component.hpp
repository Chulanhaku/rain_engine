#pragma once

#include <rain/core/math/vec3.hpp>

namespace rain
{
    struct velocity_3d_component
    {
        // Units per second. Physics uses world space; ordinary movement uses
        // transform-local space (the two coincide for root-level entities).
        vec3 linear{};
    };
}