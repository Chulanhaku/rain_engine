add_library(rain_render)

target_sources(rain_render
    PRIVATE
        src/d3d11/d3d11_render_backend.cpp

    PUBLIC
        include/rain/render/render_backend.hpp
        include/rain/render/render_clear_color.hpp
        include/rain/render/d3d11/d3d11_render_backend.hpp
)

target_include_directories(rain_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/includea
)

target_link_libraries(rain_render
    PUBLIC
        rain_core
        rain_platform

    PRIVATE
        d3d11
        dxgi
)

target_compile_features(rain_render
    PUBLIC
        cxx_std_20
)

rain_apply_common_options(rain_render)
rain_set_project_warnings(rain_render)