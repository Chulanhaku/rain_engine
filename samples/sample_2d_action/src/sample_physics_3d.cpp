#include "sample_physics_3d.hpp"
#include "sample_3d_world_builder.hpp"

#include <rain/core/log.hpp>
#include <rain/render/mesh_3d_component.hpp>
#include <rain/runtime/camera_input_3d_component.hpp>
#include <rain/runtime/camera_input_system_3d.hpp>
#include <rain/runtime/collider_3d_component.hpp>
#include <rain/runtime/movement_system_3d.hpp>
#include <rain/runtime/physics_system_3d.hpp>
#include <rain/runtime/rigid_body_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/transform_hierarchy_system_3d.hpp>
#include <rain/runtime/velocity_3d_component.hpp>
#include <rain/runtime/world_transform_3d_component.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace sample_3d {
namespace {
using namespace rain;

entity_query_desc movement_query() {
    entity_query_desc query;
    query.required_components={get_type_id<transform_3d_component>(),get_type_id<velocity_3d_component>()};
    return query;
}
void kinematic_patrol(system_context& context,void*) {
    if (!context.target_world || !context.entity_query) return;
    auto& w=*context.target_world;
    for (auto e : w.query_entities(*context.entity_query)) {
        if (!w.has_tag_in_hierarchy(e,tag_id{"physics.kinematic"}) ||
            w.has_tag_in_hierarchy(e,tag_id{"state.frozen"}) ||
            w.has_tag_in_hierarchy(e,tag_id{"physics.disabled"})) continue;
        const auto& transform=w.get_component<transform_3d_component>(e);
        auto& velocity=w.get_component<velocity_3d_component>(e);
        if (transform.position.x>=3) velocity.linear.x=-1.5f;
        if (transform.position.x<=-3) velocity.linear.x=1.5f;
    }
}
}

void physics_demo_systems::reset(world& w) {
    simulation.reset();
    fixed_state={};
    step_count=collision_enters=trigger_enters=trigger_stays=trigger_exits=0;
    entity_query_desc query;
    query.required_components={get_type_id<physics_feedback_component>()};
    query.require_active=false;
    for (auto entity : w.query_entities(query)) {
        auto& feedback=w.get_component<physics_feedback_component>(entity);
        while (feedback.trigger_contacts>0) {
            w.remove_tag(entity,tag_id{"state.in_trigger"});
            --feedback.trigger_contacts;
        }
        if (auto* mesh=w.try_get_component<mesh_3d_component>(entity)) mesh->tint=feedback.normal_tint;
    }
}

void physics_demo_systems::run_test_frame(system_scheduler& scheduler,world& w,
    event_system& events,f32 delta_seconds,u64 frame_index) {
    scheduler.run_frame(w,events,delta_seconds,frame_index,fixed_settings,fixed_state);
}

void physics_demo_systems::consume_events(system_context& context,void* user_data) {
    if (!context.target_world || !user_data) return;
    auto& physics=*static_cast<physics_demo_systems*>(user_data);
    auto& w=*context.target_world;
    ++physics.step_count;
    for (const auto& event : physics.simulation.events()) {
        if (!event.trigger) {
            if (event.type==collision_event_type_3d::enter) ++physics.collision_enters;
            continue;
        }
        if (event.type==collision_event_type_3d::enter) ++physics.trigger_enters;
        else if (event.type==collision_event_type_3d::stay) ++physics.trigger_stays;
        else ++physics.trigger_exits;
        if (physics.log_events && event.type!=collision_event_type_3d::stay)
            log_info(std::string{"Trigger "}+(event.type==collision_event_type_3d::enter ? "enter" : "exit")+
                " at physics tick "+std::to_string(event.fixed_tick_index));
        for (auto entity : {event.first,event.second}) {
            auto* feedback=w.try_get_component<physics_feedback_component>(entity);
            if (!feedback) continue;
            if (event.type==collision_event_type_3d::enter) {
                ++feedback->trigger_contacts;
                w.add_tag(entity,tag_id{"state.in_trigger"});
            } else if (event.type==collision_event_type_3d::exit && feedback->trigger_contacts>0) {
                --feedback->trigger_contacts;
                w.remove_tag(entity,tag_id{"state.in_trigger"});
            }
            if (auto* mesh=w.try_get_component<mesh_3d_component>(entity))
                mesh->tint=feedback->trigger_contacts>0 ? vec4{0.1f,1,1,1} : feedback->normal_tint;
        }
    }
    // Consume every fixed step: render frames with several steps never lose enter/exit.
    physics.simulation.clear_events();
}

void register_sample_3d_systems(system_scheduler& scheduler,
    physics_demo_systems& physics,input_action_map* input) {
    auto camera=movement_query();
    camera.required_components.push_back(get_type_id<camera_input_3d_component>());
    scheduler.add_system({.system_name="system.camera_input_3d", .owner_name="sample_2d_action",
        .phase=system_phase::input, .entity_query=camera,
        .function=camera_input_system_3d, .user_data=input});
    auto movement=movement_query();
    movement.required_tags.require_all(tag_id{"object.movable"});
    scheduler.add_system({.system_name="system.movement_3d", .owner_name="sample_2d_action",
        .phase=system_phase::movement, .entity_query=movement, .function=movement_system_3d,
        .user_data=&physics.free_movement});
    auto patrol=movement_query();
    patrol.required_tags.require_all(tag_id{"sample.kinematic_patrol"});
    scheduler.add_system({.system_name="sample.kinematic_patrol", .owner_name="sample_2d_action",
        .phase=system_phase::physics, .priority=300, .entity_query=patrol, .function=kinematic_patrol});
    scheduler.add_system({.system_name="system.movement_3d.kinematic", .owner_name="sample_2d_action",
        .phase=system_phase::physics, .priority=200, .entity_query=movement_query(), .function=movement_system_3d,
        .user_data=&physics.kinematic_movement});
    scheduler.add_system({.system_name="system.physics_world_3d", .owner_name="sample_2d_action",
        .phase=system_phase::physics, .priority=100, .entity_query={},
        .function=physics_world_system_3d, .user_data=&physics.simulation});
    scheduler.add_system({.system_name="sample.physics_events", .owner_name="sample_2d_action",
        .phase=system_phase::post_physics, .entity_query={},
        .function=physics_demo_systems::consume_events, .user_data=&physics});
    scheduler.add_system({.system_name="system.physics_query_sync_3d", .owner_name="sample_2d_action",
        .phase=system_phase::post_update, .priority=75, .entity_query={},
        .function=physics_query_sync_system_3d, .user_data=&physics.simulation});
    entity_query_desc hierarchy;
    hierarchy.required_components={get_type_id<transform_3d_component>()};
    hierarchy.required_tags.require_all(tag_id{"transform.3d"}).reject(tag_id{"transform.disabled"});
    scheduler.add_system({.system_name="system.transform_hierarchy_3d", .owner_name="sample_2d_action",
        .phase=system_phase::post_update, .priority=100,
        .entity_query=hierarchy, .function=transform_hierarchy_system_3d});
}

int run_physics_sample_tests() {
    int checks=0,failures=0;
    const auto check=[&](bool condition,const char* name) {
        ++checks;
        if (!condition) { ++failures; std::fprintf(stderr,"FAIL: %s\n",name); }
    };
    const auto near=[](f32 a,f32 b,f32 epsilon=0.002f) { return std::abs(a-b)<=epsilon; };
    world w;
    event_system events;
    system_scheduler scheduler;
    physics_demo_systems physics;
    const auto h=build_sample_3d_world(w,{});
    register_sample_3d_systems(scheduler,physics,nullptr);
    const auto y=[&](entity_id e) { return w.get_component<transform_3d_component>(e).position.y; };
    bool sphere_bounced=false, sphere_passed_trigger=false;
    for (u64 frame=0;frame<360;++frame) {
        physics.run_test_frame(scheduler,w,events,1.0f/60.0f,frame);
        sphere_bounced |= w.get_component<velocity_3d_component>(h.sphere).linear.y>0.5f;
        sphere_passed_trigger |= y(h.sphere)<1.0f;
    }
    check(physics.step_count==720,"120 Hz fixed step count");
    check(near(y(h.cube),0.65f),"dynamic cube rests on scaled ground without penetration");
    check(near(y(h.ground),-0.5f),"static ground never integrates");
    check(near(y(h.frozen_cube),3),"state.frozen suspends gravity and integration");
    check(near(y(h.floating_cube),3),"physics.no_gravity keeps floating body suspended");
    check(sphere_bounced && sphere_passed_trigger,"sphere-box collision bounces after crossing trigger");
    check(near(y(h.sphere),0.65f,0.03f),"sphere eventually settles on ground");
    check(near(y(h.kinematic_cube),0.5f),"kinematic platform ignores gravity");
    check(std::abs(w.get_component<transform_3d_component>(h.kinematic_cube).position.x+3)>1,
        "kinematic movement system patrols platform");
    check(near(w.get_component<world_transform_3d_component>(h.cube).position.y,y(h.cube)),
        "hierarchy sees this frame's physics before render prepare");

    world other;
    event_system other_events;
    system_scheduler other_scheduler;
    physics_demo_systems other_physics;
    const auto other_h=build_sample_3d_world(other,{});
    register_sample_3d_systems(other_scheduler,other_physics,nullptr);
    for (u64 frame=0;frame<180;++frame) other_physics.run_test_frame(other_scheduler,other,other_events,1.0f/30.0f,frame);
    check(other_physics.step_count==physics.step_count &&
        near(other.get_component<transform_3d_component>(other_h.sphere).position.y,y(h.sphere)) &&
        near(other.get_component<transform_3d_component>(other_h.kinematic_cube).position.x,
             w.get_component<transform_3d_component>(h.kinematic_cube).position.x),
        "30 and 60 FPS share identical fixed-step physics");

    w.remove_tag(h.frozen_cube,tag_id{"state.frozen"});
    w.remove_tag(h.floating_cube,tag_id{"physics.no_gravity"});
    for (u64 frame=360;frame<480;++frame) physics.run_test_frame(scheduler,w,events,1.0f/60.0f,frame);
    check(near(y(h.frozen_cube),0.6f),"removing frozen tag resumes falling");
    check(near(y(h.floating_cube),0.6f),"removing no_gravity tag resumes falling");
    reset_sample_3d_world(w,h);
    reset_sample_3d_world(w,h);
    check(w.tag_count(h.frozen_cube,tag_id{"state.frozen"})==1 &&
          w.tag_count(h.floating_cube,tag_id{"physics.no_gravity"})==1,
        "reset is idempotent for reference-counted tags");
    w.remove_tag(h.trigger_platform,tag_id{"physics.trigger"});
    w.get_component<rigid_body_3d_component>(h.sphere).restitution=0;
    for (u64 frame=0;frame<240;++frame) physics.run_test_frame(scheduler,w,events,1.0f/60.0f,frame);
    check(near(y(h.sphere),2.77f),"removing trigger tag makes platform solid");

    // Small arrangements exercise runtime rules beyond the visible scene.
    const auto add_body=[](world& target,vec3 position,collider_shape_3d shape,f32 inverse_mass) {
        const auto e=target.create_entity();
        target.add_component<transform_3d_component>(e,transform_3d_component{.position=position});
        target.add_component<velocity_3d_component>(e);
        target.add_component<rigid_body_3d_component>(e,rigid_body_3d_component{
            .inverse_mass=inverse_mass,.linear_damping=0,.restitution=1});
        target.add_component<collider_3d_component>(e,collider_3d_component{.shape=shape});
        target.add_tag(e,tag_id{"physics.dynamic"});
        target.add_tag(e,tag_id{"physics.no_gravity"});
        target.add_tag(e,tag_id{"object.movable"});
        target.add_tag(e,tag_id{"transform.3d"});
        return e;
    };
    world motion;
    event_system motion_events;
    system_scheduler motion_scheduler;
    physics_demo_systems motion_physics;
    register_sample_3d_systems(motion_scheduler,motion_physics,nullptr);
    const auto moving=add_body(motion,{0,0,0},collider_shape_3d::box,1);
    motion.get_component<velocity_3d_component>(moving).linear={2,0,0};
    for (u64 frame=0;frame<60;++frame) motion_physics.run_test_frame(motion_scheduler,motion,motion_events,1.0f/60.0f,frame);
    check(near(motion.get_component<transform_3d_component>(moving).position.x,2),
        "dynamic body is not integrated twice by movement and physics");
    motion.set_entity_active(moving,false);
    motion_physics.run_test_frame(motion_scheduler,motion,motion_events,0.1f,60);
    check(near(motion.get_component<transform_3d_component>(moving).position.x,2),"inactive body is excluded");
    motion.set_entity_active(moving,true);
    motion.get_component<rigid_body_3d_component>(moving).inverse_mass=0;
    motion_physics.run_test_frame(motion_scheduler,motion,motion_events,0.1f,61);
    check(near(motion.get_component<transform_3d_component>(moving).position.x,2),"zero inverse mass is immovable");
    const u64 old_steps=motion_physics.step_count;
    motion_physics.run_test_frame(motion_scheduler,motion,motion_events,-1,62);
    motion_physics.run_test_frame(motion_scheduler,motion,motion_events,std::numeric_limits<f32>::quiet_NaN(),63);
    check(motion_physics.step_count==old_steps,"invalid time does not advance physics");
    motion_physics.run_test_frame(motion_scheduler,motion,motion_events,10,64);
    check(motion_physics.step_count-old_steps==30,"long frames bound catch-up to 30 substeps");

    const auto collide=[](world& target) {
        physics_world_3d simulation;
        // The public world step includes integration; use a minimal positive step
        // to isolate the existing contact/impulse oracle without another solver API.
        simulation.step(target,0.000001f,0);
    };
    world pair;
    const auto a=add_body(pair,{0,0,0},collider_shape_3d::box,1);
    const auto b=add_body(pair,{0.8f,0,0},collider_shape_3d::box,0.5f);
    pair.get_component<velocity_3d_component>(a).linear={1,0,0};
    pair.get_component<velocity_3d_component>(b).linear={-1,0,0};
    collide(pair);
    check(near(pair.get_component<velocity_3d_component>(a).linear.x,-5.0f/3.0f) &&
          near(pair.get_component<velocity_3d_component>(b).linear.x,1.0f/3.0f),
        "unequal-mass elastic collision uses relative velocity and opposite impulses");
    check(near(pair.get_component<transform_3d_component>(a).position.x,-0.2f*2.0f/3.0f),
        "penetration correction is weighted by inverse mass");
    world coincident;
    const auto s1=add_body(coincident,{0,0,0},collider_shape_3d::sphere,1);
    const auto s2=add_body(coincident,{0,0,0},collider_shape_3d::sphere,1);
    collide(coincident);
    check(near(length(coincident.get_component<transform_3d_component>(s1).position-
                      coincident.get_component<transform_3d_component>(s2).position),1),
        "coincident spheres get a finite separating normal");
    for (bool sphere_first : {false,true}) {
        world mixed;
        entity_id ball,box;
        if (sphere_first) {
            ball=add_body(mixed,{0,0.8f,0},collider_shape_3d::sphere,1);
            box=add_body(mixed,{0,0,0},collider_shape_3d::box,0);
        } else {
            box=add_body(mixed,{0,0,0},collider_shape_3d::box,0);
            ball=add_body(mixed,{0,0.8f,0},collider_shape_3d::sphere,1);
        }
        collide(mixed);
        check(near(mixed.get_component<transform_3d_component>(ball).position.y,1),
            sphere_first ? "sphere-box normal is correct" : "box-sphere normal is correct");
        mixed.get_component<transform_3d_component>(ball).position={0,0,0};
        collide(mixed);
        check(near(length(mixed.get_component<transform_3d_component>(ball).position),1),
            "sphere centered inside box is expelled through a nearest face");
    }

    world inherited;
    event_system inherited_events;
    system_scheduler inherited_scheduler;
    physics_demo_systems inherited_physics;
    register_sample_3d_systems(inherited_scheduler,inherited_physics,nullptr);
    const auto parent=inherited.create_entity();
    inherited.add_component<transform_3d_component>(parent,transform_3d_component{
        .position={10,0,0},.rotation={0,0,1.57079632679f},.scale={2,3,1}});
    inherited.add_tag(parent,tag_id{"transform.3d"});
    inherited.add_tag(parent,tag_id{"physics.no_gravity"});
    const auto child=add_body(inherited,{0,1,0},collider_shape_3d::sphere,1);
    inherited.remove_tag(child,tag_id{"physics.no_gravity"});
    inherited.add_tag(child,tag_id{"transform.inherit_parent"});
    check(inherited.set_parent(child,parent),"physics parent setup");
    inherited.get_component<velocity_3d_component>(child).linear={1,0,0};
    for (u64 frame=0;frame<60;++frame)
        inherited_physics.run_test_frame(inherited_scheduler,inherited,inherited_events,1.0f/60.0f,frame);
    const auto child_position=inherited.get_component<world_transform_3d_component>(child).position;
    check(near(child_position.x,8) && near(child_position.y,0),
        "world-space physics velocity survives rotated scaled parent and inherited no_gravity");
    inherited.add_tag(parent,tag_id{"state.frozen"});
    inherited_physics.run_test_frame(inherited_scheduler,inherited,inherited_events,0.1f,60);
    check(near(inherited.get_component<world_transform_3d_component>(child).position.x,8),
        "parent frozen tag suspends child physics");

    inherited.remove_tag(parent,tag_id{"state.frozen"});
    inherited.add_tag(parent,tag_id{"physics.kinematic"});
    inherited.remove_tag(child,tag_id{"object.movable"});
    inherited.get_component<velocity_3d_component>(child).linear={0,1,0};
    const auto local_before=inherited.get_component<transform_3d_component>(child).position;
    inherited_physics.fixed_state.accumulator=0;
    inherited_physics.run_test_frame(inherited_scheduler,inherited,inherited_events,1.0f/240.0f,61);
    check(near(inherited.get_component<transform_3d_component>(child).position.y,local_before.y),
        "inherited kinematic body waits for a fixed step");
    inherited_physics.run_test_frame(inherited_scheduler,inherited,inherited_events,1.0f/240.0f,62);
    check(near(inherited.get_component<transform_3d_component>(child).position.y,
        local_before.y+1.0f/120.0f,0.00001f),
        "inherited kinematic body moves once at fixed rate without object.movable");

    const auto before_disabled=inherited.get_component<transform_3d_component>(child).position;
    inherited.add_tag(parent,tag_id{"physics.disabled"});
    inherited_physics.run_test_frame(inherited_scheduler,inherited,inherited_events,0.1f,63);
    check(near(inherited.get_component<transform_3d_component>(child).position.y,before_disabled.y),
        "inherited physics.disabled suspends kinematic movement");
    inherited.remove_tag(parent,tag_id{"physics.disabled"});
    inherited_physics.run_test_frame(inherited_scheduler,inherited,inherited_events,0.1f,64);
    check(near(inherited.get_component<transform_3d_component>(child).position.y,before_disabled.y+0.1f),
        "removing inherited physics.disabled resumes kinematic movement");

    camera_input_3d_component camera_controls;
    transform_3d_component camera_transform;
    velocity_3d_component camera_velocity;
    apply_camera_input_3d(camera_controls,camera_input_3d_state{.move_x=1,.move_y=1,.move_z=1},
        0.25f,camera_transform,camera_velocity);
    check(near(length(camera_velocity.linear),camera_controls.move_speed),
        "camera diagonal movement is normalized");
    camera_transform.rotation={0,1.57079632679f,0};
    apply_camera_input_3d(camera_controls,camera_input_3d_state{.move_z=1},
        0.25f,camera_transform,camera_velocity);
    check(near(camera_velocity.linear.x,camera_controls.move_speed) && near(camera_velocity.linear.z,0),
        "camera forward movement follows yaw");
    apply_camera_input_3d(camera_controls,camera_input_3d_state{.look_y=100},
        100,camera_transform,camera_velocity);
    check(near(camera_transform.rotation.x,camera_controls.max_pitch),"camera pitch is clamped");
    apply_camera_input_3d(camera_controls,camera_input_3d_state{.move_z=1,.boost=true},
        0.25f,camera_transform,camera_velocity);
    check(near(length(camera_velocity.linear),camera_controls.move_speed*camera_controls.boost_multiplier),
        "camera boost multiplies movement speed");
    apply_camera_input_3d(camera_controls,{},0.25f,camera_transform,camera_velocity);
    check(near(length(camera_velocity.linear),0),"camera input release clears velocity");

    world camera_world;
    const auto camera_entity=camera_world.create_entity();
    camera_world.add_component<transform_3d_component>(camera_entity);
    camera_world.add_component<velocity_3d_component>(camera_entity);
    camera_world.add_component<camera_input_3d_component>(camera_entity);
    camera_world.add_tag(camera_entity,tag_id{"camera.input"});
    camera_world.add_tag(camera_entity,tag_id{"camera.active"});
    auto camera_query=movement_query();
    camera_query.required_components.push_back(get_type_id<camera_input_3d_component>());
    system_context camera_context{.target_world=&camera_world,.entity_query=&camera_query,
        .phase=system_phase::input,.delta_seconds=0.25f};
    apply_camera_input_3d(camera_controls,camera_input_3d_state{.move_z=1},0.25f,
        camera_world.get_component<transform_3d_component>(camera_entity),
        camera_world.get_component<velocity_3d_component>(camera_entity));
    movement_system_3d(camera_context,nullptr);
    check(near(camera_world.get_component<transform_3d_component>(camera_entity).position.z,
        camera_controls.move_speed*0.25f),"camera input feeds movement system translation");
    input_action_map idle_input;
    camera_input_system_3d(camera_context,&idle_input);
    movement_system_3d(camera_context,nullptr);
    check(near(camera_world.get_component<transform_3d_component>(camera_entity).position.z,
        camera_controls.move_speed*0.25f),"camera input system stops movement when keys are released");
    camera_world.get_component<velocity_3d_component>(camera_entity).linear={1,0,0};
    camera_world.add_tag(camera_entity,tag_id{"state.frozen"});
    camera_input_system_3d(camera_context,&idle_input);
    movement_system_3d(camera_context,nullptr);
    check(near(camera_world.get_component<transform_3d_component>(camera_entity).position.x,0),
        "camera respects frozen tag");
    camera_world.remove_tag(camera_entity,tag_id{"state.frozen"});
    camera_world.remove_tag(camera_entity,tag_id{"camera.input"});
    camera_world.get_component<velocity_3d_component>(camera_entity).linear={1,0,0};
    camera_input_system_3d(camera_context,&idle_input);
    check(near(length(camera_world.get_component<velocity_3d_component>(camera_entity).linear),0),
        "removing camera.input clears residual movement");
    check(physics.trigger_enters>0 && physics.trigger_exits>0,"sample consumes trigger enter and exit events");
    const auto world_tests=run_physics_world_tests();
    checks+=world_tests.checks;
    failures+=world_tests.failures;
    std::printf("Physics World sample: %d checks, %d failures\n",checks,failures);
    return failures==0 ? 0 : 1;
}
}
