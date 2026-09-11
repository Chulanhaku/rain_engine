#pragma once

#include <rain/core/math/mat4.hpp>

namespace rain {
    struct camera_3d_component {
        f32 vertical_fov_radians = 1.04719755f;
        f32 near_plane = 0.1f;
        f32 far_plane = 1000.0f;
    };

    struct camera_3d_frame {
        mat4 view;
        mat4 projection;
        mat4 view_projection;
        vec3 position;

        f32 vertical_fov_radians = 1.04719755f;
        f32 aspect_ratio = 1.0f;
        f32 near_plane = 0.1f;
        f32 far_plane = 1000.0f;
    };

}
