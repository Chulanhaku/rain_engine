#pragma once

#include<rain/core/types.hpp>

namespace rain{
    struct handle{
        u32 index = invalid_u32;
        u32 generation = 0;

        [[nodiscard]] bool is_valid()const{
            return index != invalid_u32;
        }

        friend bool operator==(handle lhs,handle rhs){
            return lhs.index==rhs.index&&lhs.generation==rhs.generation;
        }
    
        friend bool operator!=(handle lhs,handle rhs){
            return !(lhs==rhs);
        }
    
    };

}