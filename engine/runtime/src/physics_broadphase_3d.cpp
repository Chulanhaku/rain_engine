#include<rain/runtime/physics_broadphase_3d.hpp>
#include<algorithm>

namespace rain {
	namespace {
		[[nodiscard]] bool overlaps_yz(const physics_aabb_3d& lhs, physics_aabb_3d& rhs) {
			if (lhs.maximum.y < rhs.minimum.y || rhs.maximum.y < lhs.minimum.y) {
				return false;
			}

			if (lhs.maximum.z < rhs.minimum.z || rhs.maximum.z < lhs.minimum.z) {
				return false;
			}

			return true;
		}

		void physics_broadphase_3d::clear() {
			proxies_.clear();
			pairs_.clear();
		}


		void physics_broadphase_3d::add(const broadphase_proxy_3d&proxy) {
			proxies_.push_back(proxy);
			
		}

		void physics_broadphase_3d::build_pairs() {
			pairs_.clear();

			std::sort(proxies_.begin(), proxies_.end(), [](const broadphase_proxy_3d& lhs, const broadphase_proxy_3d& rhs) {return lhs.bounds.minimum.x < rhs.bounds.minimum.x; });

			for (usize lhs_index = 0; lhs_index < proxies_.size(); ++lhs_index) {
				const broadphase_proxy_3d& lhs = proxies_[lhs_index];

				for (usize rhs_index = lhs_index + 1; rhs_index < proxies_.size(); ++rhs_index) {
					const broadphase_proxy_3d& rhs = proxies_[rhs_index];

					if (rhs.bounds.minimum.x > lhs.bounds.maximum.x) {
						break;
					}

					if (!lhs.dynamic && !rhs.dynamic && !lhs.trigger && !rhs.trigger) {
						continue;
					}

					if ((lhs.mask & rhs.layer) == 0 || (rhs.mask & lhs.layer) == 0)continue;

					if (!overlaps_yz(lhs.bounds, rhs.bounds))continue;

					pairs_.push_back(broadphase_pair_3d{.first = lhs.entity, .second = rhs.entity});
				}
			}
		}

		const std::vector<broadphase_pair_3d>& physics_broadphase_3d::pairs()const {
			return pairs_;
		}

		usize physics_broadphase_3dd::proxy_count()const {
			return proxies_.size();
		}

		usize physcis_broadphase_3d::pair_count()const {
			return pairs_.size();
		}
	}
}