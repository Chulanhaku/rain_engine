#pragma once

#include<rain/core/container/rain_hash_map.hpp>
#include<rain/core/tag/tag.hpp>
#include<rain/core/tag/tag_event.hpp>
#include<rain/core/types.hpp>

#include<vector>

namespace rain {
	class tag_container {
	public:
		bool add(tag_id tag) {
			if (!tag.is_valid())return false;

			u32& count = tag_counts_[tag];
			++count;

			if (count == 1) {
				pending_events_.push_back(tag_event{
					.type = tag_event_type::added,
					.tag = tag,
					.new_count = count
				});
			
				return true;
			}

			return false;

		}

		bool remove(tag_id tag) {
			u32* count = tag_counts_.find(tag);

			if (count == nullptr || *count == 0)return false;

			--(*count);

			if (*count == 0) {
				tag_counts_.erase(tag);

				pending_events_.push_back(tag_event{
					.type = tag_event_type::removed,
					.tag = tag,
					.new_count = 0
					});

				return true;

			}

			return false;
		}

		[[nodiscard]] bool has(tag_id tag)const {
			const u32* count = tag_counts_.find(tag);
			return count != nullptr && *count > 0;
		}

		[[nodiscard]] u32 count(tag_id tag)const {
			const u32* count_value = tag_counts_.find(tag);

			if (count_value == nullptr)return 0;

			return *count_value;
		}

		void clear() {
			tag_counts_.clear();
			pending_events_.clear();
		}

		[[nodiscard]] bool empty()const {
			return tag_counts_.empty();
		}

		[[nodiscard]] const std::vector<tag_event>& pending_events()const {
			return pending_events_;
		}

		void clear_pending_events() {
			pending_events_.clear();
		}

	private:
		rain_hash_map<tag_id, u32>tag_counts_;
		std::vector<tag_event>pending_events_;
	};
}