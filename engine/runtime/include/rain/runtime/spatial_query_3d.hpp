#pragma once

#include <rain/core/tag/tag_query.hpp>
#include <rain/runtime/collider_3d_component.hpp>
#include <rain/runtime/collision_filter_3d_component.hpp>
#include <rain/runtime/entity.hpp>
#include <rain/runtime/physics_broadphase_3d.hpp>
#include <optional>
#include <vector>

namespace rain {
class world;

struct ray_3d {
    vec3 origin{};
    vec3 direction{0,0,1}; // Normalized internally; distances are world units.
    f32 max_distance = 1000;
};
struct query_sphere_3d { vec3 center{}; f32 radius = 0.5f; };
// V0 translates a world-axis-aligned box; rotation during a sweep is unsupported.
struct query_box_3d { vec3 center{}; vec3 half_extents{0.5f,0.5f,0.5f}; };
enum class query_trigger_mode_3d : u8 { exclude, include, only };

struct spatial_query_filter_3d {
    u32 layer_mask = collision_layer_3d::all;
    // Zero ignores the target's simulation mask. Nonzero enables bilateral filtering.
    u32 source_layer = 0;
    query_trigger_mode_3d triggers = query_trigger_mode_3d::exclude;
    std::vector<entity_id> ignored_entities; // Complete ids, including generations.
    tag_query tags; // Exact, local entity Tags, captured at sync; no prefix inheritance.
};
struct spatial_query_hit_3d {
    entity_id entity;
    vec3 point{};  // World point on the TARGET collider, not the swept shape's center.
    vec3 normal{}; // Unit outward target normal, towards the querying shape.
    f32 distance = 0;
    f32 fraction = 0; // distance / requested max_distance; zero for overlaps.
    bool started_overlapping = false; // Includes touching at t=0; distance is then zero.
    bool trigger = false;
};

// Explicit read-only snapshot. No ECS pointers, impulses, events or hidden refresh.
// sync() rebuilds a median-split AABB tree from active, enabled colliders, including
// those without rigid bodies. Queries never mutate it and may run concurrently
// provided no sync/clear is in progress and output vectors are not shared.
// Hits can become stale after ECS changes: resync and check world::is_alive before use.
class spatial_query_3d {
public:
    void sync(const world& target_world);
    void clear();
    [[nodiscard]] usize collider_count() const { return colliders_.size(); }
    [[nodiscard]] usize node_count() const { return nodes_.size(); }
    [[nodiscard]] u64 revision() const { return revision_; }

    // Invalid input => no hit / cleared output. Dimensions and distances must be
    // finite and nonnegative. Nonzero travel requires a finite nonzero direction.
    // Zero travel is an overlap query and accepts a zero direction.
    // All-hit results overwrite the output and sort by distance, then entity id.
    // Initial overlap normals/witnesses are stable representatives, not an MTD solver.
    [[nodiscard]] std::optional<spatial_query_hit_3d> raycast(const ray_3d& ray,
        const spatial_query_filter_3d& filter = {}) const;
    [[nodiscard]] bool raycast_any(const ray_3d& ray, const spatial_query_filter_3d& filter = {}) const;
    void raycast_all(const ray_3d& ray, std::vector<spatial_query_hit_3d>& hits,
        const spatial_query_filter_3d& filter = {}) const;
    [[nodiscard]] std::optional<spatial_query_hit_3d> sweep_sphere(const query_sphere_3d& sphere,
        vec3 direction, f32 max_distance, const spatial_query_filter_3d& filter = {}) const;
    [[nodiscard]] bool sweep_sphere_any(const query_sphere_3d& sphere, vec3 direction,
        f32 max_distance, const spatial_query_filter_3d& filter = {}) const;
    void sweep_sphere_all(const query_sphere_3d& sphere, vec3 direction, f32 max_distance,
        std::vector<spatial_query_hit_3d>& hits, const spatial_query_filter_3d& filter = {}) const;
    [[nodiscard]] std::optional<spatial_query_hit_3d> sweep_box(const query_box_3d& box,
        vec3 direction, f32 max_distance, const spatial_query_filter_3d& filter = {}) const;
    [[nodiscard]] bool sweep_box_any(const query_box_3d& box, vec3 direction,
        f32 max_distance, const spatial_query_filter_3d& filter = {}) const;
    void sweep_box_all(const query_box_3d& box, vec3 direction, f32 max_distance,
        std::vector<spatial_query_hit_3d>& hits, const spatial_query_filter_3d& filter = {}) const;
    [[nodiscard]] bool overlap_sphere_any(const query_sphere_3d& sphere,
        const spatial_query_filter_3d& filter = {}) const;
    [[nodiscard]] bool overlap_box_any(const query_box_3d& box,
        const spatial_query_filter_3d& filter = {}) const;
    void overlap_sphere(const query_sphere_3d& sphere, std::vector<spatial_query_hit_3d>& hits,
        const spatial_query_filter_3d& filter = {}) const;
    void overlap_box(const query_box_3d& box, std::vector<spatial_query_hit_3d>& hits,
        const spatial_query_filter_3d& filter = {}) const;

private:
    struct collider_record {
        entity_id entity;
        collider_shape_3d shape;
        vec3 center, half_extents;
        f32 radius = 0;
        physics_aabb_3d bounds;
        collision_filter_3d_component filter;
        bool trigger = false;
        tag_container tags;
    };
    struct tree_node {
        physics_aabb_3d bounds;
        u32 begin = 0, count = 0;
        u32 left = 0, right = 0;
    };
    u32 build_node(u32 begin, u32 end);
    std::optional<spatial_query_hit_3d> cast(collider_shape_3d shape, vec3 center,
        vec3 half_extents, f32 radius, vec3 direction, f32 distance,
        const spatial_query_filter_3d& filter, std::vector<spatial_query_hit_3d>* all, bool any) const;
    std::vector<collider_record> colliders_;
    std::vector<u32> indices_;
    std::vector<tree_node> nodes_;
    u64 revision_ = 0;
};
}
