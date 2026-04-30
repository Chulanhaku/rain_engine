#pragma once

#include<rain/core/event/event_context.hpp>

#include<string>
#include<vector>

namespace{
    struct event_trace_entry{
        event_context context;
        std::string listener_name;
        std::string listener_owner;

        i32 listener_priority = 0;
    };

    class event_trace_log{
    public:
        void set_enabled(bool enabled){
            enabled_ = enabled;
        }

        [[nodiscard]]bool is_enabled()const{
            return enabled_;
        }

        void push(event_trace_entry entry){
            if(!enabled_)return;
            entries_.push_back(std::move(entry));
        }

        void clear(){
            entries_.clear();
        }

        [[nodiscard]]const std::vector<event_trace_entry>&entries()const{
            return entries_;
        }

        [[nodiscard]]usize size()const{
            return entries_.size();
        }

    private:
        bool enabled_ = true;
        std::vector<event_trace_entry> entries_;
    };

}