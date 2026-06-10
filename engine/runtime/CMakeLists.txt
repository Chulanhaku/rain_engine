add_library(rain_runtime)

target_sources(rain_runtime
    PRIVATE
        src/entity.cpp
        src/world.cpp
        src/movement_system_2d.cpp
        #src/component_storage.cpp
        #src/scene.cpp
    PUBLIC
        include/rain/runtime/entity.hpp
        include/rain/runtime/world.hpp
        include/rain/runtime/component_pool.hpp
        include/rain/runtime/component_registry.hpp
        include/rain/runtime/system_scheduler.hpp
        include/rain/runtime/transform_2d_component.hpp
        include/rain/runtime/velocity_2d_component.hpp
        include/rain/runtime/movement_system_2d.hpp
)

target_include_directories(rain_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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