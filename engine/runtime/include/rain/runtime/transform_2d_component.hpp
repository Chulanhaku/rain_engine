#pragma once

#include<rain/core/math/vec2.hpp>
#include<rain/core/types.hpp>

namespace rain{

    struct transform_2d_component{
        vec2 position;
        float rotation = 0.0f;
        vec2 scale{1.0f,1.0f};
    };
}