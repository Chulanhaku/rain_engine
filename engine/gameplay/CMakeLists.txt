add_library(rain_gameplay)

target_sources(rain_gameplay
    PRIVATE
        src/gameplay_tag.cpp
    PUBLIC
        include/rain/gameplay/gameplay_tag.hpp
        include/rain/gameplay/gameplay_tag_container.hpp
        include/rain/gameplay/gameplay_tag_event.hpp
        include/rain/gameplay/gameplay_tag_query.hpp
)

target_include_directories(rain_gameplay
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rain_gameplay
    PUBLIC
        rain_core
        rain_runtime
)

rain_apply_common_options(rain_gameplay)
rain_apply_warnings(rain_gameplay)