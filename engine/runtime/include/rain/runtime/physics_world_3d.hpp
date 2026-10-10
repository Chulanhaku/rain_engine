#pragma once

#include <rain/core/container/rain_hash_map.hpp>
#include <rain/runtime/entity.hpp>
#include <rain/runtime/physics_3d.hpp>
#include <rain/runtime/spatial_query_3d.hpp>
#include <rain/runtime/physics_broadphase_3d.hpp>
#include <functional>
#include <vector>

namespace rain {
class world;
enum class collision_event_type_3d : u8 { enter, stay, exit };
struct collision_event_3d {
    collision_event_type_3d type = collision_event_type_3d::enter;
    // Canonical order includes entity generations. Exit may contain dead ids.
    entity_id first;
    entity_id second;
    // World-space normal points from second to first.
    vec3 normal{};
    f32 penetration = 0.0f;
    bool trigger = false;
    u64 fixed_tick_index = 0;
};
struct collision_pair_key_3d {
    entity_id first;
    entity_id second;
    friend bool operator==(const collision_pair_key_3d& a,const collision_pair_key_3d& b) {
        return a.first == b.first && a.second == b.second;
    }
};
struct collision_pair_hash_3d {
    usize operator()(const collision_pair_key_3d& key) const noexcept {
        const u64 first = (static_cast<u64>(key.first.index) << 32) | key.first.generation;
        const u64 second = (static_cast<u64>(key.second.index) << 32) | key.second.generation;
        const usize a = std::hash<u64>{}(first), b = std::hash<u64>{}(second);
        return a ^ (b + usize{0x9e3779b9u} + (a << 6) + (a >> 2));
    }
};
struct active_collision_3d {
    bool trigger = false;
    vec3 normal{};
    f32 penetration = 0.0f;
};
struct physics_world_3d_stats {
    u32 collider_count = 0;
    u32 broadphase_pair_count = 0;
    // Actual narrowphase calls across all solver iterations.
    u32 narrowphase_test_count = 0;
    // Unique contacts detected during this step, including triggers.
    u32 collision_count = 0;
};

// One instance per ECS world. Call reset() before reusing with another world.
class physics_world_3d {
public:
    explicit physics_world_3d(physics_settings_3d settings = {});
    void step(world& target_world,f32 fixed_delta_seconds,u64 fixed_tick_index);
    // Events accumulate across substeps until explicitly consumed.
    [[nodiscard]] const std::vector<collision_event_3d>& events() const;
    void clear_events();
    void reset();
    // step() synchronizes after solving. Also sync before queries when editing,
    // before the first step, or after component/Tag changes outside simulation.
    void sync_queries(const world& target_world);
    [[nodiscard]] const spatial_query_3d& queries() const;
    [[nodiscard]] const physics_world_3d_stats& stats() const;
    [[nodiscard]] physics_settings_3d& settings();
    [[nodiscard]] const physics_settings_3d& settings() const;
private:
    void integrate(world& target_world,f32 delta_seconds);
    void build_broadphase(world& target_world);
    void solve_collisions(world& target_world);
    void finalize_collision_events(u64 fixed_tick_index);
    physics_settings_3d settings_;
    physics_broadphase_3d broadphase_;
    spatial_query_3d queries_;
    rain_hash_map<collision_pair_key_3d,active_collision_3d,collision_pair_hash_3d> active_collisions_;
    rain_hash_map<collision_pair_key_3d,active_collision_3d,collision_pair_hash_3d> current_collisions_;
    std::vector<collision_pair_key_3d> active_pairs_;
    std::vector<collision_pair_key_3d> current_pairs_;
    std::vector<collision_event_3d> events_;
    physics_world_3d_stats stats_;
};
}
