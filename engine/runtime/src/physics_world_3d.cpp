#include <rain/runtime/physics_world_3d.hpp>

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
        constexpr f32 contact_tolerance = 0.00001f;

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
                has_physics_tag(target_world, entity, "physics.disabled") ||
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
            if (penetration.x < -contact_tolerance || penetration.y < -contact_tolerance || penetration.z < -contact_tolerance) return {};

            if (penetration.x <= penetration.y && penetration.x <= penetration.z)
            {
                return {true, {difference.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f}, std::max(penetration.x,0.0f)};
            }
            if (penetration.y <= penetration.z)
            {
                return {true, {0.0f, difference.y < 0.0f ? -1.0f : 1.0f, 0.0f}, std::max(penetration.y,0.0f)};
            }
            return {true, {0.0f, 0.0f, difference.z < 0.0f ? -1.0f : 1.0f}, std::max(penetration.z,0.0f)};
        }

        [[nodiscard]] collision_contact_3d intersect_sphere(const world_sphere_3d& lhs, const world_sphere_3d& rhs)
        {
            const vec3 difference = lhs.center - rhs.center;
            const f32 distance_squared = length_squared(difference);
            const f32 radius_sum = lhs.radius + rhs.radius;
            if (distance_squared > (radius_sum + contact_tolerance) * (radius_sum + contact_tolerance)) return {};
            const f32 distance = std::sqrt(distance_squared);
            return {
                .colliding = true,
                .normal = distance > contact_epsilon ? difference / distance : vec3{0.0f, 1.0f, 0.0f},
                .penetration = std::max(radius_sum - distance,0.0f)
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
            if (distance_squared > (sphere.radius + contact_tolerance) * (sphere.radius + contact_tolerance)) return {};

            if (distance_squared > contact_epsilon * contact_epsilon)
            {
                const f32 distance = std::sqrt(distance_squared);
                return {true, difference / distance, std::max(sphere.radius - distance,0.0f)};
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
                has_physics_tag(target_world, entity, "physics.disabled") ||
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

    physics_world_3d::physics_world_3d(physics_settings_3d settings) : settings_(settings) {}

    void physics_world_3d::integrate(world& target_world,f32 delta_seconds) {
        entity_query_desc query;
        query.required_components = {get_type_id<transform_3d_component>(),get_type_id<velocity_3d_component>(),
            get_type_id<rigid_body_3d_component>()};
        for (entity_id entity : target_world.query_entities(query)) {
            if (dynamic_inverse_mass(target_world,entity) <= 0.0f) continue;
            auto& velocity = target_world.get_component<velocity_3d_component>(entity);
            const auto& body = target_world.get_component<rigid_body_3d_component>(entity);
            if (!has_physics_tag(target_world,entity,"physics.no_gravity"))
                velocity.linear += settings_.gravity * (body.gravity_scale * delta_seconds);
            velocity.linear *= std::exp(-std::max(body.linear_damping,0.0f) * delta_seconds);
            translate_world(target_world,entity,velocity.linear * delta_seconds);
        }
    }

    void physics_world_3d::build_broadphase(world& target_world) {
        broadphase_.clear();
        entity_query_desc query;
        query.required_components = {get_type_id<transform_3d_component>(),get_type_id<collider_3d_component>()};
        for (entity_id entity : target_world.query_entities(query)) {
            if (has_physics_tag(target_world,entity,"physics.disabled")) continue;
            const auto& collider = target_world.get_component<collider_3d_component>(entity);
            const mat4 matrix = current_world_matrix(target_world,entity);
            vec3 center,extent;
            if (collider.shape == collider_shape_3d::box) {
                const auto box = make_world_aabb(matrix,collider);
                center = box.center;extent = box.half_extents;
            } else if (collider.shape == collider_shape_3d::sphere) {
                const auto sphere = make_world_sphere(matrix,collider);
                center = sphere.center;extent = {sphere.radius,sphere.radius,sphere.radius};
            } else continue;
            // Keep narrowphase's touching tolerance inside conservative broadphase bounds.
            extent += vec3{contact_tolerance,contact_tolerance,contact_tolerance};
            const auto* filter = target_world.try_get_component<collision_filter_3d_component>(entity);
            const collision_filter_3d_component defaults;
            if (!filter) filter = &defaults;
            broadphase_.add({.entity=entity,.bounds={center-extent,center+extent},
                .layer=filter->layer,.mask=filter->mask,
                .dynamic=dynamic_inverse_mass(target_world,entity)>0.0f ||
                    (has_physics_tag(target_world,entity,"physics.kinematic") &&
                     !has_physics_tag(target_world,entity,"physics.static") &&
                     !has_physics_tag(target_world,entity,"state.frozen")),
                .trigger=has_physics_tag(target_world,entity,"physics.trigger")});
        }
        broadphase_.build_pairs();
        stats_.collider_count = static_cast<u32>(broadphase_.proxy_count());
        stats_.broadphase_pair_count = static_cast<u32>(broadphase_.pair_count());
    }

    void physics_world_3d::solve_collisions(world& target_world) {
        // Zero iterations disables response, but still detects contacts and triggers.
        const u32 passes = std::max(settings_.solver_iterations,1u);
        for (u32 iteration=0;iteration<passes;++iteration) {
            for (const auto& pair : broadphase_.pairs()) {
                const auto& lhs = target_world.get_component<collider_3d_component>(pair.first);
                const auto& rhs = target_world.get_component<collider_3d_component>(pair.second);
                ++stats_.narrowphase_test_count;
                const auto contact = intersect_colliders(current_world_matrix(target_world,pair.first),lhs,
                    current_world_matrix(target_world,pair.second),rhs);
                if (!contact.colliding) continue;
                const bool trigger = has_physics_tag(target_world,pair.first,"physics.trigger") ||
                    has_physics_tag(target_world,pair.second,"physics.trigger");
                const collision_pair_key_3d key{pair.first,pair.second};
                if (!current_collisions_.contains(key)) {
                    current_collisions_.insert(key,{trigger,contact.normal,contact.penetration});
                    current_pairs_.push_back(key);
                }
                if (!trigger && iteration<settings_.solver_iterations)
                    resolve_collision(target_world,pair.first,pair.second,contact,
                        dynamic_inverse_mass(target_world,pair.first),dynamic_inverse_mass(target_world,pair.second),settings_);
            }
        }
        stats_.collision_count = static_cast<u32>(current_collisions_.size());
    }

    void physics_world_3d::finalize_collision_events(u64 fixed_tick_index) {
        const usize first_new_event = events_.size();
        for (const auto& key : active_pairs_) {
            const auto& previous = *active_collisions_.find(key);
            const auto* current = current_collisions_.find(key);
            if (!current || current->trigger != previous.trigger)
                events_.push_back({collision_event_type_3d::exit,key.first,key.second,previous.normal,0,
                    previous.trigger,fixed_tick_index});
        }
        for (const auto& key : current_pairs_) {
            const auto& current = *current_collisions_.find(key);
            const auto* previous = active_collisions_.find(key);
            const auto type = previous && previous->trigger==current.trigger ?
                collision_event_type_3d::stay : collision_event_type_3d::enter;
            events_.push_back({type,key.first,key.second,current.normal,current.penetration,current.trigger,fixed_tick_index});
        }
        const auto entity_less=[](entity_id a,entity_id b) {
            return a.index<b.index || (a.index==b.index && a.generation<b.generation);
        };
        // Deterministic delivery; a contact-type change exits before entering its new type.
        std::sort(events_.begin()+static_cast<std::ptrdiff_t>(first_new_event),events_.end(),
            [&](const collision_event_3d& a,const collision_event_3d& b) {
                if (a.first!=b.first) return entity_less(a.first,b.first);
                if (a.second!=b.second) return entity_less(a.second,b.second);
                const auto rank=[](collision_event_type_3d type) { return type==collision_event_type_3d::exit ? 0 : 1; };
                return rank(a.type)<rank(b.type);
            });
        std::swap(active_collisions_,current_collisions_);
        active_pairs_.swap(current_pairs_);
    }

    void physics_world_3d::step(world& target_world,f32 fixed_delta_seconds,u64 fixed_tick_index) {
        if (!std::isfinite(fixed_delta_seconds) || fixed_delta_seconds<=0.0f) return;
        stats_ = {};
        current_collisions_.clear();
        current_pairs_.clear();
        integrate(target_world,fixed_delta_seconds);
        build_broadphase(target_world);
        solve_collisions(target_world);
        finalize_collision_events(fixed_tick_index);
    }
    const std::vector<collision_event_3d>& physics_world_3d::events() const { return events_; }
    void physics_world_3d::clear_events() { events_.clear(); }
    void physics_world_3d::reset() {
        broadphase_.clear();active_collisions_.clear();current_collisions_.clear();
        active_pairs_.clear();current_pairs_.clear();events_.clear();stats_={};
    }
    const physics_world_3d_stats& physics_world_3d::stats() const { return stats_; }
    physics_settings_3d& physics_world_3d::settings() { return settings_; }
    const physics_settings_3d& physics_world_3d::settings() const { return settings_; }
}
