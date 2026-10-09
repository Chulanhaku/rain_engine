#pragma once

#include <rain/core/math/vec3.hpp>
#include <rain/core/container/rain_hash_map.hpp>
#include <rain/core/types.hpp>
#include <rain/runtime/collision_filter_3d_component.hpp>
#include <rain/runtime/entity.hpp>
#include <vector>

namespace rain {
struct physics_aabb_3d {
    vec3 minimum{};
    vec3 maximum{};
};
struct broadphase_proxy_3d {
    entity_id entity;
    physics_aabb_3d bounds;
    u32 layer = collision_layer_3d::default_layer;
    u32 mask = collision_layer_3d::all;
    bool dynamic = false;
    bool trigger = false;
};
struct broadphase_pair_3d {
    entity_id first;
    entity_id second;
};

class physics_broadphase_3d {
public:
    void clear();
    // Replaces a complete entity id; invalid bounds remove its previous proxy.
    // Mutations invalidate pairs until build_pairs() is called again.
    void add(const broadphase_proxy_3d& proxy);
    void build_pairs();
    [[nodiscard]] const std::vector<broadphase_pair_3d>& pairs() const;
    [[nodiscard]] usize proxy_count() const;
    [[nodiscard]] usize pair_count() const;
private:
    std::vector<broadphase_proxy_3d> proxies_;
    rain_hash_map<u64,usize> proxy_indices_;
    std::vector<broadphase_pair_3d> pairs_;
};
}
