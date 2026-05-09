#pragma once
#include<rain/core/event/event_trace.hpp>
#include<rain/core/types.hpp>
#include<string>

namespace rain{
    struct event_trace_filter{
        std::string event_name_contains;
        std::string source_name_contains;
        std::string reason_contains;
        std::string listener_name_contains;
        std::string listener_owner_contains;

        bool use_frame_range=false;
        u64 min_frame_index=0;
        u64 max_frame_index =0;

        bool use_dispatch_mode = false;
        event_dispatch_mode dispatch_mode = event_dispatch_mode::immediate;

        [[nodiscard]]bool matched(const event_trace_entry&entry)const{
            if(!event_name_contains.empty()&&!contains(entry.context.event_name,event_name_contains) )return false;

            if (
                !source_name_contains.empty() &&
                !contains(entry.context.source_name, source_name_contains))
            {
                return false;
            }

            if (
                !reason_contains.empty() &&
                !contains(entry.context.reason, reason_contains))
            {
                return false;
            }

            if (
                !listener_name_contains.empty() &&
                !contains(entry.listener_name, listener_name_contains))
            {
                return false;
            }

            if (
                !listener_owner_contains.empty() &&
                !contains(entry.listener_owner, listener_owner_contains))
            {
                return false;
            }

            if (use_frame_range)
            {
                if (
                    entry.context.frame_index < min_frame_index ||
                    entry.context.frame_index > max_frame_index)
                {
                    return false;
                }
            }

            if (use_dispatch_mode && entry.context.mode != dispatch_mode)
            {
                return false;
            }

            return true;
        }

    private:
        [[nodiscard]] static bool contains(const std::string& text, const std::string& pattern)
        {
            return text.find(pattern) != std::string::npos;
        }
        
    };
}