add_library(rain_core)

target_sources(rain_core
    PRIVATE
        src/assert.cpp
        src/log.cpp
        src/string_id.cpp
        src/allocator.cpp
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