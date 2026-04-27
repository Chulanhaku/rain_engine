add_library(rain_platform)

target_sources(rain_platform
    PRIVATE
        src/timer.cpp
)

target_include_directories(rain_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rain_platform
    PUBLIC
        rain_core
)

rain_apply_common_options(rain_platform)
rain_apply_warnings(rain_platform)