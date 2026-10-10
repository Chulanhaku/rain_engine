#pragma once

// Shared V0 shape conversion for simulation and scene queries. Keep both paths
// on the same current local transforms, conservative AABBs and scaled spheres.
#include <rain/runtime/collider_3d_component.hpp>
#include <rain/runtime/local_matrix_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/world.hpp>
#include <algorithm>
#include <cmath>

namespace rain::detail {
        // Match transform_hierarchy_system_3d's explicit inherit-parent opt-in,
        // and compute from current local transforms, not last frame's cache.
        [[nodiscard]] inline mat4 current_world_matrix(const world& target_world, entity_id entity)
        {
            mat4 result = mat4::identity();
            usize visited = 0;
            while (target_world.is_alive(entity) && visited++ < target_world.entity_capacity())
            {
                const auto* transform = target_world.try_get_component<transform_3d_component>(entity);
                if (transform == nullptr) break;
                const auto* matrix_override = target_world.try_get_component<local_matrix_3d_component>(entity);
                result = result * (matrix_override != nullptr ? matrix_override->matrix : transform->matrix());

                if (!target_world.has_tag(entity, tag_id{"transform.inherit_parent"})) break;
                const entity_id parent = target_world.parent_of(entity);
                if (!target_world.is_entity_active(parent)) break;
                entity = parent;
            }
            return result;
        }

        [[nodiscard]] inline vec3 basis(const mat4& matrix, u32 row)
        {
            return {matrix.values[row][0], matrix.values[row][1], matrix.values[row][2]};
        }

        struct world_aabb_3d
        {
            vec3 center{};
            vec3 half_extents{};
        };

        [[nodiscard]] inline world_aabb_3d make_world_aabb(const mat4& matrix, const collider_3d_component& collider)
        {
            const vec3 half{
                std::abs(collider.half_extents.x),
                std::abs(collider.half_extents.y),
                std::abs(collider.half_extents.z)
            };
            return {
                .center = transform_point(collider.center, matrix),
                .half_extents = {
                    std::abs(matrix.values[0][0]) * half.x + std::abs(matrix.values[1][0]) * half.y + std::abs(matrix.values[2][0]) * half.z,
                    std::abs(matrix.values[0][1]) * half.x + std::abs(matrix.values[1][1]) * half.y + std::abs(matrix.values[2][1]) * half.z,
                    std::abs(matrix.values[0][2]) * half.x + std::abs(matrix.values[1][2]) * half.y + std::abs(matrix.values[2][2]) * half.z
                }
            };
        }

        struct world_sphere_3d
        {
            vec3 center{};
            f32 radius = 0.0f;
        };

        [[nodiscard]] inline world_sphere_3d make_world_sphere(const mat4& matrix, const collider_3d_component& collider)
        {
            // Bound the largest eigenvalue of A*A^T. Unlike max_basis_scale,
            // this stays conservative when parent nonuniform scale adds shear.
            const vec3 x = basis(matrix, 0);
            const vec3 y = basis(matrix, 1);
            const vec3 z = basis(matrix, 2);
            const f32 bound_squared = std::max({
                dot(x, x) + std::abs(dot(x, y)) + std::abs(dot(x, z)),
                dot(y, y) + std::abs(dot(y, x)) + std::abs(dot(y, z)),
                dot(z, z) + std::abs(dot(z, x)) + std::abs(dot(z, y))
            });
            return {
                .center = transform_point(collider.center, matrix),
                .radius = std::abs(collider.radius) * std::sqrt(std::max(bound_squared, 0.0f))
            };
        }

}
