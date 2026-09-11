#pragma once

#include <rain/render/bounds_3d.hpp>
#include <rain/render/camera_3d.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace rain {
    struct frustum_plane_3d {
        vec3 normal{};
        f32 distance = 0.0f;
    };

    struct camera_frustum_3d {
        std::array<frustum_plane_3d, 6> planes{};
    };

    // Row-vector matrices, D3D clip volume: -w <= x,y <= w, 0 <= z <= w.
    [[nodiscard]] inline camera_frustum_3d extract_camera_frustum(const mat4& view_projection) {
        camera_frustum_3d result{};
        const auto& m = view_projection.values;
        for (usize index = 0; index < result.planes.size(); ++index) {
            const usize axis = index / 2;
            const f32 sign = index % 2 == 0 ? 1.0f : -1.0f;
            const f32 w = index == 4 ? 0.0f : 1.0f;
            auto& plane = result.planes[index];
            plane.normal = {w * m[0][3] + sign * m[0][axis],
                            w * m[1][3] + sign * m[1][axis],
                            w * m[2][3] + sign * m[2][axis]};
            plane.distance = w * m[3][3] + sign * m[3][axis];
            const f32 normal_length = length(plane.normal);
            if (normal_length > 0.0f && std::isfinite(normal_length)) {
                plane.normal = plane.normal * (1.0f / normal_length);
                plane.distance /= normal_length;
            } else {
                // Degenerate planes must never discard geometry.
                plane = {};
            }
        }
        return result;
    }

    [[nodiscard]] inline bounding_sphere_3d transform_bounding_sphere(
        const bounding_sphere_3d& local_bounds, const mat4& world_matrix) {
        // sqrt(max absolute row sum of A*A^T) bounds the largest singular
        // value. Unlike the longest basis vector, this also handles shear
        // introduced by rotated children under nonuniformly scaled parents.
        f32 scale_squared = 0.0f;
        for (usize row = 0; row < 3; ++row) {
            f32 row_sum = 0.0f;
            for (usize column = 0; column < 3; ++column) {
                f32 entry = 0.0f;
                for (usize k = 0; k < 3; ++k)
                    entry += world_matrix.values[row][k] * world_matrix.values[column][k];
                row_sum += std::abs(entry);
            }
            if (!std::isfinite(row_sum))
                return {transform_point(local_bounds.center, world_matrix),
                        std::numeric_limits<f32>::infinity()};
            scale_squared = std::max(scale_squared, row_sum);
        }
        return {transform_point(local_bounds.center, world_matrix),
                local_bounds.radius * std::sqrt(scale_squared)};
    }

    [[nodiscard]] inline bool sphere_inside_camera_frustum(
        const bounding_sphere_3d& bounds, const camera_frustum_3d& frustum) {
        if (!std::isfinite(bounds.radius) || bounds.radius < 0.0f ||
            !std::isfinite(bounds.center.x) || !std::isfinite(bounds.center.y) ||
            !std::isfinite(bounds.center.z)) return true;
        for (const auto& plane : frustum.planes) {
            const f32 distance = dot(plane.normal, bounds.center) + plane.distance;
            // Keep tangency and absorb float roundoff at large coordinates.
            const f32 tolerance = 1.0e-5f * (1.0f + bounds.radius + std::abs(plane.distance));
            if (distance < -bounds.radius - tolerance) return false;
        }
        return true;
    }

    [[nodiscard]] inline bool sphere_inside_camera_frustum(
        const bounding_sphere_3d& bounds, const camera_3d_frame& camera) {
        return sphere_inside_camera_frustum(bounds, extract_camera_frustum(camera.view_projection));
    }
}
