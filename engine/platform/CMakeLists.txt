find_package(glfw3 CONFIG REQUIRED)
add_library(rain_platform)

target_sources(rain_platform
    PRIVATE
        src/timer.cpp
        src/window.cpp
        src/input_action.cpp
    PUBLIC 
        include/rain/platform/key_code.hpp
        include/rain/platform/window.hpp
        include/rain/platform/input_action.hpp
)

target_include_directories(rain_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rain_platform
    PUBLIC
        rain_core
        glfw
)

rain_apply_common_options(rain_platform)
rain_set_project_warnings(rain_platform)