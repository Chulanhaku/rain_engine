#pragma once

#include <rain/gameplay/gameplay_tag_container.hpp>

namespace rain{
    struct gameplay_tag_query{
        std::vector<gameplay_tag> all;
        std::vector<gameplay_tag> any;
        std::vector<gameplay_tag> none;

        [[nodiscard]] bool matches(const gameplay_tag_container& container)const{
            if(!container.has_all(all))return false;
            if(!any.empty()&&!container.has_any(any))return false;
            if(container.has_any(none))return false;

            return true;
        }

    };
}