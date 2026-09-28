#pragma once

#include <rain/runtime/camera_input_3d_component.hpp>
#include <rain/runtime/system_scheduler.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/velocity_3d_component.hpp>

namespace rain {
struct camera_input_3d_state {
    f32 move_x=0,move_y=0,move_z=0;
    f32 look_x=0,look_y=0;
    bool boost=false;
};

// Pure input mapping, also usable by replays and headless sample checks.
void apply_camera_input_3d(const camera_input_3d_component& settings,
    const camera_input_3d_state& input,f32 delta_seconds,
    transform_3d_component& transform,velocity_3d_component& velocity);

// user_data: input_action_map*, nullptr means no input. Query all entities with
// camera_input + transform + velocity; tag checks are internal so disabling
// camera.input/camera.active clears any remaining velocity.
void camera_input_system_3d(system_context& context,void* user_data);
}
