#pragma once

#include<source_location>
#include<string>

#include<rain/core/event/event_fwd.hpp>

namespace rain{
    struct event_emit_desc{
        std::string source_name;
        std::string reason;
        std::source_location location = std::source_location::current();
    };

    struct event_context{
        event_dispatch_mode mode = event_dispatch_mode::immediate;

        std::string event_name;
        std::string source_name;
        std::string reason;

        const char* file_name = "";
        const char* function_name="";
        u32 line = 0;
        
        u64 sequence = 0;
        u64 frame_index =0;
    };

    [[nodiscard]] inline event_context make_event_context(event_dispatch_mode mode,const std:;string&event_name,const event_emit_desc&desc,u64 sequence,u64 frame_index){
            return event_context{
            .mode = mode,
            .event_name = event_name,
            .source_name = desc.source_name,
            .reason = desc.reason,
            .file_name = desc.location.file_name(),
            .function_name = desc.location.function_name(),
            .line = static_cast<u32>(desc.location.line()),
            .sequence = sequence,
            .frame_index = frame_index
        };
    }

    
}