add_library(rain_runtime)

target_sources(rain_runtime
    PRIVATE
        src/entity.cpp
        src/world.cpp
        src/component_storage.cpp
        src/scene.cpp
)

target_include_directories(rain_runtime
    PUBLIC
        $(CMAKE_CURRENT_SOURCE_DIR)/include
)

target_link_libraries(rain_runtime
    PUBLIC
        rain_core
        rain_platform
)

target_compile_features(rain_runtime
    PUBLIC
        cxx_std_20
)

rain_set_project_warnings(rain_runtime)
rain_apply_common_options(rain_runtime)