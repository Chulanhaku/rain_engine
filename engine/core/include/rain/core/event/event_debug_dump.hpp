#pragma once
#include <rain/core/event/event_system.hpp>

#include<rain/core/event/event_trace_filter.hpp>

#include<sstream>
#include<string>

namespace rain{
    struct event_debug_dump_options{
        bool include_event_registry = true;
        bool include_listener_table = true;
        bool include_trace = true;

        event_trace_filter trace_filter;
    };

    [[nodiscard]]inline const char* to_string(event_dispatch_mode mode){
        switch(mode){
            case event_dispatch_mode::immediate:return"immediate";
            case event_dispatch_mode::queued:return"queued";
        }

        return "unknown";
    }

    [[nodiscard]] inline std::string dump_event_registry(const event_system&events){
        std::ostringstream output;
        output<<"[event registry]\n";

        const std::vector<event_type_debug_info>event_infos = events.get_event_debug_infos();

        for(const event_type_debug_info&info:event_infos){
                output
                    << "event=" << info.event_name
                    << ", category=" << info.category
                    << ", type_id=" << info.id.value
                    << ", event_size=" << info.event_size
                    << ", listeners=" << info.listener_count
                    << ", queued=" << info.queued_count
                    << ", allow_immediate=" << (info.allow_immediate_dispatch ? "true" : "false")
                    << ", allow_queued=" << (info.allow_queued_dispatch ? "true" : "false")
                    << ", trace=" << (info.trace_enabled ? "true" : "false")
                    << "\n";
        }

        return output.str();
    }

    [[nodiscard]] inline std::string dump_event_listener_table(const event_system& events){
        std::ostringstream output;

        output << "[event listener table]\n";

        const std::vector<event_listener_table_entry> listener_infos =
            events.get_all_listener_debug_infos();

        for (const event_listener_table_entry& info : listener_infos)
        {
            output
                << "event=" << info.event_name
                << ", category=" << info.category
                << ", listener=" << info.listener_name
                << ", owner=" << info.owner_name
                << ", priority=" << info.priority
                << ", alive=" << (info.alive ? "true" : "false")
                << ", enabled=" << (info.enabled ? "true" : "false")
                << ", generation=" << info.generation
                << "\n";
        }

        return output.str();
    }

    [[nodiscard]] inline std::string dump_event_trace(
        const event_system& events,
        const event_trace_filter& filter = {})
    {
        std::ostringstream output;

        output << "[event trace]\n";

        const std::vector<event_trace_entry>& entries =
            events.trace_log().entries();

        for (const event_trace_entry& entry : entries)
        {
            if (!filter.matched(entry))
            {
                continue;
            }

            output
                << "seq=" << entry.context.sequence
                << ", frame=" << entry.context.frame_index
                << ", mode=" << to_string(entry.context.mode)
                << ", event=" << entry.context.event_name
                << ", source=" << entry.context.source_name
                << ", listener=" << entry.listener_name
                << ", owner=" << entry.listener_owner
                << ", priority=" << entry.listener_priority
                << ", file=" << entry.context.file_name
                << ", line=" << entry.context.line
                << ", function=" << entry.context.function_name
                << ", reason=" << entry.context.reason
                << "\n";
        }

        return output.str();
    }

    [[nodiscard]]inline std::string dump_event_debug_report(const event_system&events,const event_debug_dump_options&options={}){
        std::ostringstream output;
        output<<"rain event debug report\n";
        output<<"=========================\n\n";
        if(options.include_event_registry){
            output<<dump_event_registry(events)<<"\n";
        }

        if(options.include_listener_table){
            output<<dump_event_listener_table(events)<<"\n";
        }

        if(options.include_trace){
            output<<dump_event_trace(events,options.trace_filter)<<"\n";
        }

        return output.str();
    }
}