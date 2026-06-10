add_library(rain_render)

target_sources(rain_render
    PRIVATE
        src/d3d11/d3d11_render_backend.cpp
        src/sprite_renderer_2d.cpp
        src/render_system_2d.cpp

    PUBLIC
        include/rain/render/render_backend.hpp
        include/rain/render/render_clear_color.hpp
        include/rain/render/d3d11/d3d11_render_backend.hpp
        include/rain/render/render_handles.hpp
        include/rain/render/render_resource_desc.hpp
        include/rain/render/sprite_renderer_2d.hpp
        include/rain/render/camera_2d.hpp
        include/rain/render/render_system_2d.hpp
        include/rain/render/sprite_2d_component.hpp
)


target_include_directories(rain_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rain_render
    PUBLIC
        rain_core
        rain_platform
        rain_runtime
    PRIVATE
        d3d11
        dxgi
        d3dcompiler
)

target_compile_features(rain_render
    PUBLIC
        cxx_std_20
)

rain_apply_common_options(rain_render)
rain_set_project_warnings(rain_render)