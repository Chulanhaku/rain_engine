#pragma once

#include<rain/core/event/event_system.hpp>
#include<rain/core/event/event_trace_filter.hpp>

#include<cctype>
#include<sstream>
#include<string>

namespace rain{
    struct event_graph_export_options{
        event_trace_filter trace_filter;

        bool include_reason_label = true;
        bool include_listener_owner = true;
        bool include_sequence = true;
    }

    [[nodiscard]]inline u64 event_graph_hash_string (const std::string&text){
        u64 hash = 14695981039346656037ull;
        for(char c:text){
            hash^=static_cast<unsigned char>(c);
            hash*= 1099511628211ull;
        }
        return hash;
    }

    [[nodiscard]]inline std::string event_graph_escape_label(const std::string&text){
        std::string result;
        result.reserve(text.size());

        for(char c:text){
            if(c=='"'||c=='\\'){
                result.push_back('\\');
            }

            if(c=='\n'||c=='\r'){
                result.push_back(' ');
                continue;
            }

            result.push_back(c);
        }

        return result;
    }

    [[nodiscard]]inline std::string event_graph_node_id(const std::string&prefix,const std::string &label){
        std::ostringstream output;

        output<<prefix
            <<"_"
            <<event_graph_hash_string(label);

        return output.str();
    }

    [[nodiscard]]inline std::string export_event_trace_dot(const event_system&events,const event_graph_export_options&options={}){
        std::ostringstream output;

        output << "digraph rain_event_trace {\n";
        output << "    rankdir=LR;\n";
        output << "    node [shape=box];\n";
        output << "\n";

        const std::vector<event_trace_entry>& entries =
            events.trace_log().entries();

        for (const event_trace_entry& entry : entries)
        {
            if (!options.trace_filter.matches(entry))
            {
                continue;
            }

            const std::string source_label =
                "source: " + entry.context.source_name;

            const std::string event_label =
                "event: " + entry.context.event_name;

            std::string listener_label =
                "listener: " + entry.listener_name;

            if (options.include_listener_owner)
            {
                listener_label += "\\nowner: " + entry.listener_owner;
            }

            const std::string source_id =
                event_graph_node_id("source", source_label);

            const std::string event_id =
                event_graph_node_id("event", event_label);

            const std::string listener_id =
                event_graph_node_id("listener", listener_label);

            output
                << "    " << source_id
                << " [label=\"" << event_graph_escape_label(source_label) << "\"];\n";

            output
                << "    " << event_id
                << " [label=\"" << event_graph_escape_label(event_label) << "\"];\n";

            output
                << "    " << listener_id
                << " [label=\"" << event_graph_escape_label(listener_label) << "\"];\n";

            std::string emit_edge_label;

            if (options.include_sequence)
            {
                emit_edge_label += "seq=" + std::to_string(entry.context.sequence);
            }

            if (options.include_reason_label && !entry.context.reason.empty())
            {
                if (!emit_edge_label.empty())
                {
                    emit_edge_label += "\\n";
                }

                emit_edge_label += entry.context.reason;
            }

            output
                << "    " << source_id
                << " -> " << event_id
                << " [label=\"" << event_graph_escape_label(emit_edge_label) << "\"];\n";

            output
                << "    " << event_id
                << " -> " << listener_id
                << " [label=\"priority="
                << entry.listener_priority
                << "\"];\n";

            output << "\n";
        }

        output << "}\n";

        return output.str();
    }

}