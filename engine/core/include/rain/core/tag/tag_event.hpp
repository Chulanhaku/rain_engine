#pragma once

#include<rain/core/tag/tag.hpp>
#include<rain/core/types.hpp>

namespace rain {
	enum class tag_event_type :u8 {
		added,
		removed
	};

	struct tag_event {
		tag_event_type type = tag_event_type::added;
		tag_id tag;
		u32 new_count;
	};
}