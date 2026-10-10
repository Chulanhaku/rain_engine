#pragma once
#include "sample_3d_world_builder.hpp"
#include <rain/runtime/physics_world_3d.hpp>
#include <rain/runtime/system_scheduler.hpp>
#include <rain/core/math/vec4.hpp>

namespace rain { class rain_window; class render_backend; class input_action_map; }
namespace sample_3d {
enum class spatial_demo_mode { ray, sphere, box };
struct spatial_query_demo {
    rain::physics_world_3d* physics=nullptr;
    rain::rain_window* window=nullptr;
    rain::render_backend* renderer=nullptr;
    rain::input_action_map* input=nullptr;
    rain::entity_id camera,selected;
    spatial_demo_mode mode=spatial_demo_mode::ray;
    bool include_triggers=false;
    bool log_selection=true;
    std::optional<rain::spatial_query_hit_3d> last_hit;
    rain::u64 query_count=0;
    // Scripted graphical smoke uses the same projection/query/highlight system.
    bool smoke_mode=false;
    rain::entity_id smoke_target;
    rain::u32 smoke_modes_hit=0;

    void initialize(rain::world& world,const sample_3d_world_resources& resources);
    void reset(rain::world& world);
    // Shared by the interactive system and headless regression tests.
    // nullptr ray hides the preview; click on a valid ray with no hit clears selection.
    void evaluate(rain::world& world,const rain::ray_3d* ray,bool clicked);
    static void run(rain::system_context& context,void* user_data);
private:
    void restore_tint(rain::world& world);
    void select(rain::world& world,rain::entity_id entity);
    rain::entity_id path_marker,probe_marker,point_marker,normal_marker;
    sample_3d_world_resources resources_;
    std::vector<rain::spatial_query_hit_3d> candidates_;
    rain::vec4 selected_tint_{1,1,1,1};
    bool previous_mouse_down_=false;
};
[[nodiscard]] int run_spatial_query_tests();
}
