#include <rain/runtime/camera_input_system_3d.hpp>

#include <rain/platform/input_action.hpp>
#include <rain/runtime/local_matrix_3d_component.hpp>

#include <algorithm>
#include <cmath>

namespace rain {
namespace {
f32 finite_or_zero(f32 value) { return std::isfinite(value) ? value : 0.0f; }
f32 axis(f32 value) { return std::clamp(finite_or_zero(value),-1.0f,1.0f); }
}

void apply_camera_input_3d(const camera_input_3d_component& settings,
    const camera_input_3d_state& input,f32 delta_seconds,
    transform_3d_component& transform,velocity_3d_component& velocity) {
    velocity.linear={};
    if (!std::isfinite(delta_seconds) || delta_seconds<=0) return;
    constexpr f32 pi=3.14159265359f;
    const f32 look_speed=std::max(finite_or_zero(settings.look_speed),0.0f);
    const f32 max_pitch=std::clamp(finite_or_zero(settings.max_pitch),0.0f,pi*0.499f);
    transform.rotation.y=std::remainder(transform.rotation.y+axis(input.look_x)*look_speed*delta_seconds,2*pi);
    transform.rotation.x=std::clamp(transform.rotation.x+axis(input.look_y)*look_speed*delta_seconds,-max_pitch,max_pitch);
    const f32 yaw=transform.rotation.y,pitch=transform.rotation.x;
    const vec3 right{std::cos(yaw),0,-std::sin(yaw)};
    const vec3 forward{std::sin(yaw)*std::cos(pitch),-std::sin(pitch),std::cos(yaw)*std::cos(pitch)};
    vec3 direction=right*axis(input.move_x)+forward*axis(input.move_z)+vec3{0,axis(input.move_y),0};
    if (length_squared(direction)>1) direction=normalize(direction);
    const f32 speed=std::max(finite_or_zero(settings.move_speed),0.0f);
    const f32 boost=input.boost ? std::max(finite_or_zero(settings.boost_multiplier),0.0f) : 1.0f;
    velocity.linear=direction*(speed*boost);
}

void camera_input_system_3d(system_context& context,void* user_data) {
    if (!context.target_world || !context.entity_query) return;
    auto& w=*context.target_world;
    const auto* input=static_cast<const input_action_map*>(user_data);
    for (auto entity : w.query_entities(*context.entity_query)) {
        auto* transform=w.try_get_component<transform_3d_component>(entity);
        auto* velocity=w.try_get_component<velocity_3d_component>(entity);
        const auto* settings=w.try_get_component<camera_input_3d_component>(entity);
        if (!transform || !velocity || !settings) continue;
        // Never overwrite the velocity of an entity owned by the physics solver.
        if (w.has_tag_in_hierarchy(entity,tag_id{"physics.dynamic"})) continue;
        velocity->linear={};
        if (!w.has_tag_in_hierarchy(entity,tag_id{"camera.input"}) ||
            !w.has_tag_in_hierarchy(entity,tag_id{"camera.active"}) ||
            w.has_tag_in_hierarchy(entity,tag_id{"state.frozen"}) ||
            w.has_tag_in_hierarchy(entity,tag_id{"state.stunned"}) ||
            w.has_tag_in_hierarchy(entity,tag_id{"state.rooted"}) ||
            w.has_tag_in_hierarchy(entity,tag_id{"physics.static"}) ||
            w.has_component<local_matrix_3d_component>(entity)) continue;
        camera_input_3d_state state;
        if (input) {
            state.move_x=input->get_axis(settings->move_x_action);
            state.move_y=input->get_axis(settings->move_y_action);
            state.move_z=input->get_axis(settings->move_z_action);
            state.look_x=input->get_axis(settings->look_x_action);
            state.look_y=input->get_axis(settings->look_y_action);
            state.boost=input->is_down(settings->boost_action);
        }
        apply_camera_input_3d(*settings,state,context.delta_seconds,*transform,*velocity);
    }
}
}
