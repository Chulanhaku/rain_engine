#pragma once

#include <rain/core/math/vec4.hpp>
#include <rain/platform/input_action.hpp>
#include <rain/runtime/fixed_step.hpp>
#include <rain/runtime/physics_world_3d.hpp>
#include <rain/runtime/movement_system_3d.hpp>
#include <rain/runtime/system_scheduler.hpp>

namespace sample_3d {
struct physics_feedback_component {
    rain::vec4 normal_tint{1,1,1,1};
    rain::u32 trigger_contacts=0;
};

// The layer owns this object for as long as its registered systems exist.
struct physics_demo_systems {
    rain::physics_world_3d simulation;
    rain::movement_settings_3d kinematic_movement{rain::movement_target_3d::kinematic};
    rain::movement_settings_3d free_movement{rain::movement_target_3d::non_kinematic};
    // Headless tests use exactly the same frame runner as application::run.
    rain::fixed_step_settings fixed_settings;
    rain::fixed_step_state fixed_state;
    rain::u64 step_count=0;
    rain::u64 collision_enters=0,trigger_enters=0,trigger_stays=0,trigger_exits=0;
    bool log_events=false;

    physics_demo_systems()=default;
    physics_demo_systems(const physics_demo_systems&)=delete;
    physics_demo_systems& operator=(const physics_demo_systems&)=delete;
    void reset(rain::world& target_world);
    void run_test_frame(rain::system_scheduler& scheduler,rain::world& target_world,
        rain::event_system& events,rain::f32 delta_seconds,rain::u64 frame_index);
    static void consume_events(rain::system_context& context,void* user_data);
};

struct physics_sample_test_result { int checks=0,failures=0; };
[[nodiscard]] physics_sample_test_result run_physics_world_tests();
void register_sample_3d_systems(rain::system_scheduler& scheduler,
    physics_demo_systems& physics,rain::input_action_map* input);
[[nodiscard]] int run_physics_sample_tests();
}
