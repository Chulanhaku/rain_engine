#pragma once

#include <rain/core/string_id.hpp>
#include <rain/core/types.hpp>

namespace rain {
// Input writes velocity; movement_system_3d owns translation. Use on a camera
// with transform + velocity, camera.input + camera.active, and no dynamic body.
struct camera_input_3d_component {
    string_id move_x_action{"camera.move_x"};
    string_id move_y_action{"camera.move_y"};
    string_id move_z_action{"camera.move_z"};
    string_id look_x_action{"camera.look_x"};
    string_id look_y_action{"camera.look_y"};
    string_id boost_action{"camera.boost"};
    f32 move_speed=6.0f;
    f32 look_speed=1.5f;
    f32 boost_multiplier=3.0f;
    f32 max_pitch=1.553343f;
};
}
