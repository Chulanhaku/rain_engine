#pragma once

#include<rain/core/types.hpp>

namespace rain{
    struct entity_id{
        u32 index = invalid_u32;
        u32 generation = 0;

        [[nodiscard]] bool is_valid()const{
            return index!= invalid_u32;
        }

        friend bool operator==(entity_id lhs,entity_id rhs){
            return lhs.index == rhs.index && lhs.generation == rhs.generation;
        }

        friend bool operator!=(entity_id lhs,entity_id rhs){
            return !(lhs== rhs);
        }
    };
}