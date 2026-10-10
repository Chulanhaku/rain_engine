#pragma once

#include <rain/render/render_handles.hpp>
#include <rain/runtime/world.hpp>

namespace sample_3d {
struct sample_3d_world_resources {
    rain::mesh_3d_handle cube_mesh;
    rain::material_3d_handle cube_material;
    rain::mesh_3d_handle sphere_mesh;
};

struct sample_3d_world_handles {
    rain::entity_id camera, cube, directional_light;
    rain::entity_id ground, frozen_cube, floating_cube, kinematic_cube;
    rain::entity_id sphere, trigger_platform;
    rain::entity_id filtered_cube, filter_platform;
    rain::entity_id rough_cube, slippery_cube;
};

[[nodiscard]] sample_3d_world_handles build_sample_3d_world(
    rain::world& target_world, const sample_3d_world_resources& resources);
void reset_sample_3d_world(rain::world& target_world, const sample_3d_world_handles& handles);
// Restart just the two friction comparison bodies, preserving the current camera.
void restart_friction_demo(rain::world& target_world, const sample_3d_world_handles& handles);
}
