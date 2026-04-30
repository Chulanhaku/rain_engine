#pragma once
#include<rain/core/types.hpp>
#include<rain/core/type_id.hpp>

namespace rain{
    class event_system;

    enum class event_dispatch_mode :u8{
        immediate,
        queued
    };

    struct event_lisner_handle{
        type_id event_type;
        u32 index = invalid_u32;
        u32 generation = 0;

        [[nodiscard]] bool is_valid()const{
            return event_type.is_valid&&index!= invalid_u32;
        }

        friend bool operator==(event_lisner_handle lhs,event_lisner_handle rhs){
            return lhs.event_type == rhs.event_type && lhs.index==rhs.index&&lhs.generation==rhs.generation;
        }

        friend bool operator!=(event_lisner_handle lhs,event_lisner_handle rhs){
            return !(lhs==rhs);
        }
    };
}