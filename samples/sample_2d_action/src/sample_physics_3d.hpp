#pragma once

#include <rain/platform/input_action.hpp>
#include <rain/runtime/physics_3d.hpp>
#include <rain/runtime/movement_system_3d.hpp>
#include <rain/runtime/system_scheduler.hpp>

namespace sample_3d {
// The layer owns this object for as long as its registered systems exist.
struct physics_demo_systems {
    rain::physics_settings_3d settings;
    rain::movement_settings_3d kinematic_movement{rain::movement_target_3d::kinematic};
    rain::movement_settings_3d free_movement{rain::movement_target_3d::non_kinematic};
    rain::system_scheduler fixed_scheduler;
    double accumulator=0;
    rain::u64 step_count=0;

    physics_demo_systems();
    physics_demo_systems(const physics_demo_systems&)=delete;
    physics_demo_systems& operator=(const physics_demo_systems&)=delete;
    static void advance(rain::system_context& context,void* user_data);
};

void register_sample_3d_systems(rain::system_scheduler& scheduler,
    physics_demo_systems& physics,rain::input_action_map* input);
[[nodiscard]] int run_physics_sample_tests();
}
