#pragma once

#include<rain/core/tag/tag_container.hpp>
#include<rain/core/tag/tag.hpp>

#include<vector>

namespace rain {
	class tag_query {
	public:
		tag_query& require_all(tag_id tag) {
			all_tags_.push_back(tag);
			return *this;
		}

		tag_query& require_any(tag_id tag){
			any_tags_.push_back(tag);
			return *this;
		}

		tag_query& reject(tag_id tag) {
			none_tags_.push_back(tag);
			return *this;
		}

		[[nodiscard]] bool matches(const tag_container& container)const {
			for (tag_id tag : all_tags_) {
				if (!container.has(tag))return false;
			}

			if (!any_tags_.empty()) {
				bool matched_any = false;

				for (tag_id tag : any_tags_) {
					if (container.has(tag)) {
						matched_any = true;
						break;
					}
				}

				if (!matched_any) {
					return false;
				}
			}

			for (tag_id tag : none_tags_) {
				if (container.has(tag)) {
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] const std::vector<tag_id>& all_tags()const {
			return all_tags_;
		}

		[[nodiscard]] const std::vector<tag_id>& any_tags()const {
			return any_tags_;
		}

		[[nodiscard]] const std::vector<tag_id>& none_tags()const {
			return none_tags_;
		}

		void clear() {
			all_tags_.clear();
			any_tags_.clear();
			none_tags_.clear();
		}

	private:
		std::vector<tag_id>all_tags_;
		std::vector<tag_id>any_tags_;
		std::vector<tag_id>none_tags_;
	};
}