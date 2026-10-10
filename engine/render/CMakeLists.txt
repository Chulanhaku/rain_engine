set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_3d.hlsl"
)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/shaders/mesh_3d.hlsl" RAIN_MESH_3D_SHADER_SOURCE)
configure_file(
    src/mesh_3d_shader_source.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/rain/render/mesh_3d_shader_source.hpp
    @ONLY
)

add_library(rain_render)

target_sources(rain_render
    PRIVATE
        src/d3d11/d3d11_render_backend.cpp
        src/d3d11/d3d11_frame_targets.cpp
        src/sprite_renderer_2d.cpp
        src/render_system_2d.cpp
        src/material_2d_registry.cpp
        src/render_queue_2d.cpp
        src/mesh_3d_registry.cpp
        src/material_3d_registry.cpp
        src/render_system_3d.cpp
        src/picking_3d.cpp
        shaders/mesh_3d.hlsl
        src/mesh_3d_shader_source.hpp.in

    PUBLIC
        include/rain/render/render_backend.hpp
        include/rain/render/render_clear_color.hpp
        include/rain/render/d3d11/d3d11_render_backend.hpp
        include/rain/render/d3d11/d3d11_frame_targets.hpp
        include/rain/render/d3d11/d3d11_rasterizer_state.hpp
        include/rain/render/render_handles.hpp
        include/rain/render/render_resource_desc.hpp
        include/rain/render/sprite_renderer_2d.hpp
        include/rain/render/camera_2d.hpp
        include/rain/render/render_system_2d.hpp
        include/rain/render/sprite_2d_component.hpp
        include/rain/render/material_2d.hpp
        include/rain/render/material_2d_registry.hpp
        include/rain/render/render_types.hpp
        include/rain/render/render_command_2d.hpp
        include/rain/render/render_queue_2d.hpp
        include/rain/render/bounds_3d.hpp
        include/rain/render/frustum_culling_3d.hpp
        include/rain/render/camera_3d.hpp
        include/rain/render/picking_3d.hpp
        include/rain/render/directional_light_3d_component.hpp
        include/rain/render/material_3d.hpp
        include/rain/render/material_3d_registry.hpp
        include/rain/render/mesh_3d.hpp
        include/rain/render/mesh_3d_component.hpp
        include/rain/render/mesh_3d_registry.hpp
        include/rain/render/render_command_3d.hpp
        include/rain/render/render_system_3d.hpp
)


target_include_directories(rain_render
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated
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