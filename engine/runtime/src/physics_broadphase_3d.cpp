#include <rain/runtime/physics_broadphase_3d.hpp>
#include <algorithm>
#include <cmath>

namespace rain {
namespace {
u64 entity_key(entity_id entity) { return (static_cast<u64>(entity.index)<<32) | entity.generation; }
bool entity_less(entity_id a, entity_id b) {
    return a.index < b.index || (a.index == b.index && a.generation < b.generation);
}
bool valid_bounds(const physics_aabb_3d& bounds) {
    const auto& lo = bounds.minimum;
    const auto& hi = bounds.maximum;
    return std::isfinite(lo.x) && std::isfinite(lo.y) && std::isfinite(lo.z) &&
        std::isfinite(hi.x) && std::isfinite(hi.y) && std::isfinite(hi.z) &&
        lo.x <= hi.x && lo.y <= hi.y && lo.z <= hi.z;
}
bool overlaps(const physics_aabb_3d& a, const physics_aabb_3d& b) {
    return a.minimum.x <= b.maximum.x && a.maximum.x >= b.minimum.x &&
        a.minimum.y <= b.maximum.y && a.maximum.y >= b.minimum.y &&
        a.minimum.z <= b.maximum.z && a.maximum.z >= b.minimum.z;
}
}
void physics_broadphase_3d::clear() {
    proxies_.clear();
    proxy_indices_.clear();
    pairs_.clear();
}
void physics_broadphase_3d::add(const broadphase_proxy_3d& proxy) {
    pairs_.clear();
    if (!proxy.entity.is_valid()) return;
    const u64 key = entity_key(proxy.entity);
    const auto* found = proxy_indices_.find(key);
    if (!valid_bounds(proxy.bounds)) {
        if (found) {
            const usize index = *found;
            if (index+1 != proxies_.size()) {
                proxies_[index] = proxies_.back();
                proxy_indices_.insert(entity_key(proxies_[index].entity),index);
            }
            proxies_.pop_back();
            proxy_indices_.erase(key);
        }
        return;
    }
    if (found) proxies_[*found] = proxy;
    else {
        proxy_indices_.insert(key,proxies_.size());
        proxies_.push_back(proxy);
    }
}
void physics_broadphase_3d::build_pairs() {
    pairs_.clear();
    std::vector<usize> order(proxies_.size());
    for (usize i=0;i<order.size();++i) order[i]=i;
    std::sort(order.begin(),order.end(),[&](usize lhs,usize rhs) {
        const auto& a=proxies_[lhs];const auto& b=proxies_[rhs];
        if (a.bounds.minimum.x != b.bounds.minimum.x) return a.bounds.minimum.x < b.bounds.minimum.x;
        return entity_less(a.entity,b.entity);
    });
    for (usize i = 0; i < proxies_.size(); ++i) {
        const auto& a = proxies_[order[i]];
        for (usize j = i + 1; j < proxies_.size(); ++j) {
            const auto& b = proxies_[order[j]];
            if (b.bounds.minimum.x > a.bounds.maximum.x) break;
            if (!a.dynamic && !b.dynamic && !a.trigger && !b.trigger) continue;
            if (!collision_filters_match({a.layer,a.mask},{b.layer,b.mask}) || !overlaps(a.bounds,b.bounds)) continue;
            if (entity_less(b.entity,a.entity)) pairs_.push_back({b.entity,a.entity});
            else pairs_.push_back({a.entity,b.entity});
        }
    }
    std::sort(pairs_.begin(),pairs_.end(),[](const broadphase_pair_3d& a,const broadphase_pair_3d& b) {
        if (a.first != b.first) return entity_less(a.first,b.first);
        return entity_less(a.second,b.second);
    });
}
const std::vector<broadphase_pair_3d>& physics_broadphase_3d::pairs() const { return pairs_; }
usize physics_broadphase_3d::proxy_count() const { return proxies_.size(); }
usize physics_broadphase_3d::pair_count() const { return pairs_.size(); }
}
