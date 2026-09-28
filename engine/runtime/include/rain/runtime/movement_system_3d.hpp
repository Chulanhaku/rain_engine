#pragma once

#include <rain/runtime/system_scheduler.hpp>

namespace rain
{
    enum class movement_target_3d { all, non_kinematic, kinematic };

    struct movement_settings_3d {
        movement_target_3d target=movement_target_3d::all;
    };

    // user_data optionally points to movement_settings_3d. Filtering uses
    // inherited physics tags, so fixed and variable steps can share this system.
    // Moves non-physics objects and kinematic bodies. Dynamic bodies are
    // integrated only by physics_integrate_system_3d, even if queries overlap.
    void movement_system_3d(system_context& context, void* user_data);
}