#pragma once

#include <rain/runtime/system_scheduler.hpp>

namespace rain
{
    // Run integration followed by collision at a fixed timestep. Both systems
    // accept physics_settings_3d through user_data (nullptr uses defaults).
    // Queries should select components; physics tags are checked in hierarchy.
    //void physics_integrate_system_3d(system_context& context, void* user_data);
    //void physics_collision_system_3d(system_context& context, void* user_data);
    // this two rebuild as physics_world_system_3d
    void physics_world_system_3d(system_context& context, void* user_data);
}