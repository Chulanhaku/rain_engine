#pragma once

#include<rain/core/tag/tag_query.hpp>
#include<rain/core/type_id.hpp>
#include<rain/core/types.hpp>
#include<rain/runtime/entity.hpp>

#include<vector>

namespace rain {
	struct entity_query_desc {
		std::vector<type_id> required_components;
		tag_query required_tags;
		bool require_alive = true;
		bool require_active = true;
	};

	struct entity_query_result {
		std::vector<entity_id> entities;

		[[nodiscard]] bool empty()const {
			return entities.empty();
		}

		[[nodiscard]] usize size()const {
			return entities.size();
		}

		[[nodiscard]] const entity_id* begin()const {
			return entities.data();
		}

		[[nodiscard]] const entity_id* end()const {
			return entities.data() + entities.size();
		}
	};
}