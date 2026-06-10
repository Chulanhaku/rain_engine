add_library(rain_core)

target_sources(rain_core
    PRIVATE
        #src/assert.cpp
        src/log.cpp
        src/string_id.cpp
        #src/allocator.cpp
    PUBLIC
        include/rain/core/assert.hpp
        include/rain/core/types.hpp
        include/rain/core/log.hpp
        include/rain/core/string_id.hpp
        include/rain/core/type_id.hpp
        include/rain/core/handle.hpp

        include/rain/core/math/vec2.hpp

        include/rain/core/container/rain_sparse_set.hpp
        include/rain/core/container/rain_vector.hpp
        include/rain/core/container/rain_hash_map.hpp
        include/rain/core/container/handle_pool.hpp

        include/rain/core/event/event_fwd.hpp
        include/rain/core/event/event_context.hpp
        include/rain/core/event/event_trace.hpp
        include/rain/core/event/event_trace_filter.hpp
        include/rain/core/event/event_trace_scope.hpp
        include/rain/core/event/event_system.hpp
        include/rain/core/event/event_debug_dump.hpp
        include/rain/core/event/event_graph_export.hpp
)

target_include_directories(rain_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(rain_core
    PUBLIC 
        cxx_std_20
)

rain_set_project_warnings(rain_core)
rain_apply_common_options(rain_core)