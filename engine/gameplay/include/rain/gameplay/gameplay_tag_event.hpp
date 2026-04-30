#pragma once
#include <rain/gameplay/gameplay_tag.hpp>

namespace rain{
    enum class gameplay_tag_event_type{
        added,
        removed
    };

    struct gameplay_tag_event{
        gameplay_tag_event_type type = gameplay_tag_event_type::added;
        gameplay_tag tag;
        u32 new_count = 0;
    };
}