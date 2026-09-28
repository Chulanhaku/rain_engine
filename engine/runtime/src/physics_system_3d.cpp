#include <rain/runtime/physics_system_3d.hpp>

#include <rain/runtime/collider_3d_component.hpp>
#include <rain/runtime/local_matrix_3d_component.hpp>
#include <rain/runtime/physics_3d.hpp>
#include <rain/runtime/rigid_body_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/velocity_3d_component.hpp>
#include <rain/runtime/world.hpp>

#include <algorithm>
#include <cmath>

namespace rain
{
    namespace
    {
        constexpr f32 contact_epsilon = 0.000001f;

        [[nodiscard]] const physics_settings_3d& resolve_physics_settings(void* user_data)
        {
            static const physics_settings_3d defaults{};
            return user_data != nullptr ? *static_cast<const physics_settings_3d*>(user_data) : defaults;
        }

        [[nodiscard]] bool has_physics_tag(const world& target_world, entity_id entity, const char* name)
        {
            return target_world.has_tag_in_hierarchy(entity, tag_id{name});
        }

        // Match transform_hierarchy_system_3d's explicit inherit-parent opt-in,
        // and compute from current local transforms, not last frame's cache.
        [[nodiscard]] mat4 current_world_matrix(const world& target_world, entity_id entity)
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

        [[nodiscard]] mat4 parent_world_matrix(const world& target_world, entity_id entity)
        {
            const entity_id parent = target_world.parent_of(entity);
            if (target_world.has_tag(entity, tag_id{"transform.inherit_parent"}) &&
                target_world.is_entity_active(parent))
            {
                return current_world_matrix(target_world, parent);
            }
            return mat4::identity();
        }

        [[nodiscard]] vec3 basis(const mat4& matrix, u32 row)
        {
            return {matrix.values[row][0], matrix.values[row][1], matrix.values[row][2]};
        }

        [[nodiscard]] f32 basis_determinant(const mat4& matrix)
        {
            return dot(basis(matrix, 0), cross(basis(matrix, 1), basis(matrix, 2)));
        }

        void translate_world(world& target_world, entity_id entity, vec3 displacement)
        {
            const mat4 parent = parent_world_matrix(target_world, entity);
            const vec3 axis_x = basis(parent, 0);
            const vec3 axis_y = basis(parent, 1);
            const vec3 axis_z = basis(parent, 2);
            const f32 determinant = dot(axis_x, cross(axis_y, axis_z));
            if (!std::isfinite(determinant) || std::abs(determinant) <= contact_epsilon) return;

            auto& transform = target_world.get_component<transform_3d_component>(entity);
            transform.position += vec3{
                dot(displacement, cross(axis_y, axis_z)) / determinant,
                dot(displacement, cross(axis_z, axis_x)) / determinant,
                dot(displacement, cross(axis_x, axis_y)) / determinant
            };
        }

        [[nodiscard]] f32 dynamic_inverse_mass(const world& target_world, entity_id entity)
        {
            if (!has_physics_tag(target_world, entity, "physics.dynamic") ||
                has_physics_tag(target_world, entity, "physics.static") ||
                has_physics_tag(target_world, entity, "physics.kinematic") ||
                has_physics_tag(target_world, entity, "state.frozen") ||
                target_world.has_component<local_matrix_3d_component>(entity) ||
                !target_world.has_component<velocity_3d_component>(entity))
            {
                return 0.0f;
            }

            const auto* body = target_world.try_get_component<rigid_body_3d_component>(entity);
            if (body == nullptr || !std::isfinite(body->inverse_mass) || body->inverse_mass <= 0.0f)
            {
                return 0.0f;
            }
            const f32 determinant = basis_determinant(parent_world_matrix(target_world, entity));
            return std::isfinite(determinant) && std::abs(determinant) > contact_epsilon ?
                body->inverse_mass : 0.0f;
        }

        struct world_aabb_3d
        {
            vec3 center{};
            vec3 half_extents{};
        };

        [[nodiscard]] world_aabb_3d make_world_aabb(const mat4& matrix, const collider_3d_component& collider)
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

        [[nodiscard]] world_sphere_3d make_world_sphere(const mat4& matrix, const collider_3d_component& collider)
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

        struct collision_contact_3d
        {
            bool colliding = false;
            // Always points from rhs toward lhs.
            vec3 normal{};
            f32 penetration = 0.0f;
        };

        [[nodiscard]] collision_contact_3d intersect_aabb(const world_aabb_3d& lhs, const world_aabb_3d& rhs)
        {
            const vec3 difference = lhs.center - rhs.center;
            const vec3 penetration{
                lhs.half_extents.x + rhs.half_extents.x - std::abs(difference.x),
                lhs.half_extents.y + rhs.half_extents.y - std::abs(difference.y),
                lhs.half_extents.z + rhs.half_extents.z - std::abs(difference.z)
            };
            if (penetration.x <= 0.0f || penetration.y <= 0.0f || penetration.z <= 0.0f) return {};

            if (penetration.x <= penetration.y && penetration.x <= penetration.z)
            {
                return {true, {difference.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f}, penetration.x};
            }
            if (penetration.y <= penetration.z)
            {
                return {true, {0.0f, difference.y < 0.0f ? -1.0f : 1.0f, 0.0f}, penetration.y};
            }
            return {true, {0.0f, 0.0f, difference.z < 0.0f ? -1.0f : 1.0f}, penetration.z};
        }

        [[nodiscard]] collision_contact_3d intersect_sphere(const world_sphere_3d& lhs, const world_sphere_3d& rhs)
        {
            const vec3 difference = lhs.center - rhs.center;
            const f32 distance_squared = length_squared(difference);
            const f32 radius_sum = lhs.radius + rhs.radius;
            if (distance_squared >= radius_sum * radius_sum) return {};
            const f32 distance = std::sqrt(distance_squared);
            return {
                .colliding = true,
                .normal = distance > contact_epsilon ? difference / distance : vec3{0.0f, 1.0f, 0.0f},
                .penetration = radius_sum - distance
            };
        }

        [[nodiscard]] collision_contact_3d intersect_sphere_box(const world_sphere_3d& sphere, const world_aabb_3d& box)
        {
            const vec3 offset = sphere.center - box.center;
            const vec3 closest{
                std::clamp(offset.x, -box.half_extents.x, box.half_extents.x),
                std::clamp(offset.y, -box.half_extents.y, box.half_extents.y),
                std::clamp(offset.z, -box.half_extents.z, box.half_extents.z)
            };
            const vec3 difference = offset - closest;
            const f32 distance_squared = length_squared(difference);
            if (distance_squared >= sphere.radius * sphere.radius) return {};

            if (distance_squared > contact_epsilon * contact_epsilon)
            {
                const f32 distance = std::sqrt(distance_squared);
                return {true, difference / distance, sphere.radius - distance};
            }

            // A center inside the box must exit through its nearest face.
            const vec3 face_distance{
                box.half_extents.x - std::abs(offset.x),
                box.half_extents.y - std::abs(offset.y),
                box.half_extents.z - std::abs(offset.z)
            };
            if (face_distance.x <= face_distance.y && face_distance.x <= face_distance.z)
            {
                return {true, {offset.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f}, sphere.radius + face_distance.x};
            }
            if (face_distance.y <= face_distance.z)
            {
                return {true, {0.0f, offset.y < 0.0f ? -1.0f : 1.0f, 0.0f}, sphere.radius + face_distance.y};
            }
            return {true, {0.0f, 0.0f, offset.z < 0.0f ? -1.0f : 1.0f}, sphere.radius + face_distance.z};
        }

        [[nodiscard]] collision_contact_3d intersect_colliders(
            const mat4& lhs_matrix, const collider_3d_component& lhs,
            const mat4& rhs_matrix, const collider_3d_component& rhs)
        {
            if (lhs.shape == collider_shape_3d::box && rhs.shape == collider_shape_3d::box)
            {
                return intersect_aabb(make_world_aabb(lhs_matrix, lhs), make_world_aabb(rhs_matrix, rhs));
            }
            if (lhs.shape == collider_shape_3d::sphere && rhs.shape == collider_shape_3d::sphere)
            {
                return intersect_sphere(make_world_sphere(lhs_matrix, lhs), make_world_sphere(rhs_matrix, rhs));
            }
            if (lhs.shape == collider_shape_3d::sphere && rhs.shape == collider_shape_3d::box)
            {
                return intersect_sphere_box(make_world_sphere(lhs_matrix, lhs), make_world_aabb(rhs_matrix, rhs));
            }
            if (lhs.shape == collider_shape_3d::box && rhs.shape == collider_shape_3d::sphere)
            {
                auto result = intersect_sphere_box(make_world_sphere(rhs_matrix, rhs), make_world_aabb(lhs_matrix, lhs));
                result.normal = -result.normal;
                return result;
            }
            return {};
        }

        [[nodiscard]] vec3 contact_velocity(const world& target_world, entity_id entity, f32 inverse_mass)
        {
            if (has_physics_tag(target_world, entity, "state.frozen") ||
                has_physics_tag(target_world, entity, "physics.static"))
            {
                return {};
            }
            const auto* velocity = target_world.try_get_component<velocity_3d_component>(entity);
            if (velocity == nullptr) return {};
            if (inverse_mass > 0.0f) return velocity->linear;
            if (has_physics_tag(target_world, entity, "physics.kinematic") &&
                !target_world.has_component<local_matrix_3d_component>(entity))
            {
                const mat4 parent = parent_world_matrix(target_world, entity);
                return basis(parent, 0) * velocity->linear.x +
                    basis(parent, 1) * velocity->linear.y + basis(parent, 2) * velocity->linear.z;
            }
            return {};
        }

        [[nodiscard]] f32 restitution_of(const world& target_world, entity_id entity)
        {
            const auto* body = target_world.try_get_component<rigid_body_3d_component>(entity);
            return body != nullptr && std::isfinite(body->restitution) ?
                std::clamp(body->restitution, 0.0f, 1.0f) : 0.0f;
        }

        void resolve_collision(world& target_world, entity_id lhs, entity_id rhs,
            const collision_contact_3d& contact, f32 lhs_mass, f32 rhs_mass, const physics_settings_3d& settings)
        {
            const f32 inverse_mass_sum = lhs_mass + rhs_mass;
            if (inverse_mass_sum <= 0.0f) return;

            const vec3 correction = contact.normal * (contact.penetration / inverse_mass_sum);
            if (lhs_mass > 0.0f) translate_world(target_world, lhs, correction * lhs_mass);
            if (rhs_mass > 0.0f) translate_world(target_world, rhs, correction * -rhs_mass);

            const vec3 relative_velocity =
                contact_velocity(target_world, lhs, lhs_mass) - contact_velocity(target_world, rhs, rhs_mass);
            const f32 normal_speed = dot(relative_velocity, contact.normal);
            if (normal_speed >= 0.0f) return;

            const f32 restitution = -normal_speed > std::max(settings.restitution_velocity_threshold, 0.0f) ?
                std::max(restitution_of(target_world, lhs), restitution_of(target_world, rhs)) : 0.0f;
            const vec3 impulse = contact.normal * (-(1.0f + restitution) * normal_speed / inverse_mass_sum);
            if (lhs_mass > 0.0f)
            {
                target_world.get_component<velocity_3d_component>(lhs).linear += impulse * lhs_mass;
            }
            if (rhs_mass > 0.0f)
            {
                target_world.get_component<velocity_3d_component>(rhs).linear -= impulse * rhs_mass;
            }
        }
    }

    void physics_integrate_system_3d(system_context& context, void* user_data)
    {
        if (context.target_world == nullptr || context.entity_query == nullptr ||
            !std::isfinite(context.delta_seconds) || context.delta_seconds <= 0.0f)
        {
            return;
        }

        world& target_world = *context.target_world;
        const auto& settings = resolve_physics_settings(user_data);
        const entity_query_result entities = target_world.query_entities(*context.entity_query);
        for (entity_id entity : entities)
        {
            if (!target_world.has_component<transform_3d_component>(entity) ||
                dynamic_inverse_mass(target_world, entity) <= 0.0f)
            {
                continue;
            }
            auto& velocity = target_world.get_component<velocity_3d_component>(entity);
            const auto& body = target_world.get_component<rigid_body_3d_component>(entity);
            if (!has_physics_tag(target_world, entity, "physics.no_gravity"))
            {
                velocity.linear += settings.gravity * (body.gravity_scale * context.delta_seconds);
            }
            // Exponential damping remains nonnegative at every timestep.
            velocity.linear *= std::exp(-std::max(body.linear_damping, 0.0f) * context.delta_seconds);
            translate_world(target_world, entity, velocity.linear * context.delta_seconds);
        }
    }

    void physics_collision_system_3d(system_context& context, void* user_data)
    {
        if (context.target_world == nullptr || context.entity_query == nullptr) return;
        world& target_world = *context.target_world;
        const auto& settings = resolve_physics_settings(user_data);
        const entity_query_result entities = target_world.query_entities(*context.entity_query);
        for (u32 iteration = 0; iteration < settings.solver_iterations; ++iteration)
        {
            for (usize lhs_index = 0; lhs_index < entities.size(); ++lhs_index)
            {
                const entity_id lhs = entities.entities[lhs_index];
                const auto* lhs_collider = target_world.try_get_component<collider_3d_component>(lhs);
                if (lhs_collider == nullptr || !target_world.has_component<transform_3d_component>(lhs) ||
                    has_physics_tag(target_world, lhs, "physics.trigger"))
                {
                    continue;
                }
                for (usize rhs_index = lhs_index + 1; rhs_index < entities.size(); ++rhs_index)
                {
                    const entity_id rhs = entities.entities[rhs_index];
                    const auto* rhs_collider = target_world.try_get_component<collider_3d_component>(rhs);
                    if (rhs_collider == nullptr || !target_world.has_component<transform_3d_component>(rhs) ||
                        has_physics_tag(target_world, rhs, "physics.trigger"))
                    {
                        continue;
                    }
                    const f32 lhs_mass = dynamic_inverse_mass(target_world, lhs);
                    const f32 rhs_mass = dynamic_inverse_mass(target_world, rhs);
                    if (lhs_mass <= 0.0f && rhs_mass <= 0.0f) continue;
                    const collision_contact_3d contact = intersect_colliders(
                        current_world_matrix(target_world, lhs), *lhs_collider,
                        current_world_matrix(target_world, rhs), *rhs_collider);
                    if (contact.colliding)
                    {
                        resolve_collision(target_world, lhs, rhs, contact, lhs_mass, rhs_mass, settings);
                    }
                }
            }
        }
    }

    void physics_world_system_3d(system_context& context, void* user_data) {
        if (context.target_world == nullptr || user_data = nullptr)return;

        auto* physics = static_cast <physics_world_3d*>(user_data);

        physics->step(*context.target_world, context.delta_seconds, context.fixed_tick_index);
    
    }


    [[nodsicard]] physics_aabb_3d make_broadphase_aabb(const transform_3d_component& transform, const collider_3d_component& collider) {
        if (collider.shape == collider_shape_3d:; box) {
            const world_aabb_3d aabb = make_world_aabb(transform, collider);

            return { .minimum = aabb.center - aabb.half_extents,.maximum = aabb.center + aabb.half_extents };
        }

        const mat4 matrix = transform.matrix();
        const vec3 center = transform_point(collider.center.matrix);

        const f32 radius = collider.radius * max_basis_scale(matrix);

        const vec3 extent{ radius,radius,radius };

        return{ .minimum = center - extent, .maximum = center + extent };
}