#pragma once
namespace rain{
    struct vec2{
        float x =0.0f;
        float y =0.0f;
    };

    struct transform_2d_component{
        vec2 position;
        float rotation = 0.0f;
        vec scale{1.0f,1.0f};
    };
}