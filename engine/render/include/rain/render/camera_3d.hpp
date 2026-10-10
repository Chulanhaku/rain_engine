#pragma once

#include <rain/core/math/mat4.hpp>
#include <rain/runtime/world_transform_3d_component.hpp>

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

    // Shared by rendering and picking: use the same orthonormal view basis.
    [[nodiscard]] inline bool build_camera_frame_3d(const world_transform_3d_component& transform,
        const camera_3d_component& camera, f32 aspect_ratio, camera_3d_frame& result) {
        result = {};
        if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0 ||
            !std::isfinite(camera.vertical_fov_radians) || camera.vertical_fov_radians <= 0 ||
            camera.vertical_fov_radians >= 3.14159265f || !std::isfinite(camera.near_plane) ||
            !std::isfinite(camera.far_plane) || camera.near_plane <= 0 || camera.far_plane <= camera.near_plane ||
            !std::isfinite(length_squared(transform.forward)) || length_squared(transform.forward) < 1e-12f ||
            !std::isfinite(length_squared(cross(transform.up,transform.forward))) ||
            length_squared(cross(transform.up,transform.forward)) < 1e-12f) return false;
        result.vertical_fov_radians=camera.vertical_fov_radians;
        result.aspect_ratio=aspect_ratio;
        result.near_plane=camera.near_plane;result.far_plane=camera.far_plane;
        result.position=transform.position;
        result.view=make_look_at_lh(transform.position,transform.position+transform.forward,transform.up);
        result.projection=make_perspective_fov_lh(camera.vertical_fov_radians,aspect_ratio,camera.near_plane,camera.far_plane);
        result.view_projection=result.view*result.projection;
        for (const auto& row:result.view_projection.values) for (f32 value:row)
            if (!std::isfinite(value)) { result={};return false; }
        return true;
    }
}
