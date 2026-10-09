#include "sample_physics_3d.hpp"
#include "sample_3d_world_builder.hpp"

#include <rain/render/mesh_3d_component.hpp>
#include <rain/runtime/collider_3d_component.hpp>
#include <rain/runtime/collision_filter_3d_component.hpp>
#include <rain/runtime/physics_broadphase_3d.hpp>
#include <rain/runtime/physics_system_3d.hpp>
#include <rain/runtime/rigid_body_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/velocity_3d_component.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <utility>
#include <vector>

namespace sample_3d {
namespace {
using namespace rain;

entity_id body(world& w,vec3 position,bool dynamic=true,bool trigger=false) {
    const auto e=w.create_entity();
    w.add_component<transform_3d_component>(e,transform_3d_component{.position=position});
    w.add_component<collider_3d_component>(e);
    w.add_component<collision_filter_3d_component>(e);
    w.add_component<rigid_body_3d_component>(e,rigid_body_3d_component{
        .inverse_mass=dynamic ? 1.0f : 0.0f,.linear_damping=0});
    w.add_component<velocity_3d_component>(e);
    w.add_tag(e,tag_id{dynamic ? "physics.dynamic" : "physics.static"});
    w.add_tag(e,tag_id{"physics.no_gravity"});
    if (trigger) w.add_tag(e,tag_id{"physics.trigger"});
    return e;
}

bool has_event(const physics_world_3d& simulation,collision_event_type_3d type,
    entity_id a,entity_id b,bool trigger) {
    for (const auto& e : simulation.events())
        if (e.type==type && e.trigger==trigger &&
            ((e.first==a && e.second==b) || (e.first==b && e.second==a))) return true;
    return false;
}

struct tick_record {
    system_phase phase;
    system_tick_type type;
    f32 dt,alpha;
    u64 frame,tick;
};
void record_tick(system_context& context,void* user_data) {
    static_cast<std::vector<tick_record>*>(user_data)->push_back({context.phase,context.tick_type,
        context.delta_seconds,context.interpolation_alpha,context.frame_index,context.fixed_tick_index});
}
}

physics_sample_test_result run_physics_world_tests() {
    physics_sample_test_result result;
    const auto check=[&](bool condition,const char* name) {
        ++result.checks;
        if (!condition) { ++result.failures; std::fprintf(stderr,"FAIL: %s\n",name); }
    };
    const auto near=[](f32 a,f32 b,f32 epsilon=0.0001f) { return std::abs(a-b)<=epsilon; };
    const collision_filter_3d_component left{.layer=1,.mask=2},right{.layer=2,.mask=1};
    check(collision_layer_3d::all==std::numeric_limits<u32>::max(),"all collision layers is a full-width bit mask");
    check(collision_filters_match(left,right),"filters compare each mask to the other layer");
    check(!collision_filters_match(left,{.layer=2,.mask=0}),"both collision masks must accept");
    check(!collision_filters_match({.layer=0,.mask=collision_layer_3d::all},right),"zero layer is filtered out");

    // An independent brute-force oracle validates sweep-and-prune candidates.
    std::vector<broadphase_proxy_3d> proxies;
    u32 seed=1234567;
    const auto random_coordinate=[&]() {
        seed=seed*1664525u+1013904223u;
        return static_cast<f32>((seed>>8)%100u)*0.12f;
    };
    for (u32 i=0;i<80;++i) {
        const vec3 p{random_coordinate(),random_coordinate(),random_coordinate()};
        proxies.push_back({.entity={i,i%3},.bounds={p,p+vec3{2,2,2}},
            .layer=1u<<(i%3),.mask=i%4==0 ? 1u : 7u,.dynamic=i%3==0,.trigger=i%11==0});
    }
    using pair_key=std::pair<u64,u64>;
    const auto id=[](entity_id e) { return (static_cast<u64>(e.index)<<32)|e.generation; };
    std::set<pair_key> expected;
    for (usize i=0;i<proxies.size();++i) for (usize j=i+1;j<proxies.size();++j) {
        const auto& a=proxies[i];const auto& b=proxies[j];
        const bool overlap=a.bounds.minimum.x<=b.bounds.maximum.x && b.bounds.minimum.x<=a.bounds.maximum.x &&
            a.bounds.minimum.y<=b.bounds.maximum.y && b.bounds.minimum.y<=a.bounds.maximum.y &&
            a.bounds.minimum.z<=b.bounds.maximum.z && b.bounds.minimum.z<=a.bounds.maximum.z;
        if (overlap && (a.dynamic || b.dynamic || a.trigger || b.trigger) &&
            (a.mask & b.layer)!=0 && (b.mask & a.layer)!=0) expected.emplace(id(a.entity),id(b.entity));
    }
    physics_broadphase_3d broadphase;
    for (const auto& proxy : proxies) broadphase.add(proxy);
    broadphase.build_pairs();
    std::set<pair_key> actual;
    std::vector<pair_key> first_order;
    for (const auto& p : broadphase.pairs()) {
        actual.emplace(id(p.first),id(p.second));first_order.emplace_back(id(p.first),id(p.second));
    }
    check(!expected.empty() && actual==expected,"broadphase matches independent brute-force spatial/filter oracle");
    check(broadphase.pair_count()==actual.size(),"broadphase contains no duplicate pairs");
    broadphase.clear();
    for (auto it=proxies.rbegin();it!=proxies.rend();++it) broadphase.add(*it);
    broadphase.build_pairs();
    std::vector<pair_key> second_order;
    for (const auto& p : broadphase.pairs()) second_order.emplace_back(id(p.first),id(p.second));
    check(first_order==second_order,"broadphase pair order is independent of insertion order");
    broadphase.clear();
    const broadphase_proxy_3d proxy{.entity={1,0},.bounds={{0,0,0},{1,1,1}},.dynamic=true};
    broadphase.add(proxy);broadphase.add(proxy);
    broadphase.add({.entity={2,0},.bounds={{1,0,0},{2,1,1}}});
    broadphase.add({.entity={3,0},.bounds={{std::numeric_limits<f32>::quiet_NaN(),0,0},{2,1,1}}});
    broadphase.add({.entity={},.bounds={{0,0,0},{1,1,1}}});
    broadphase.build_pairs();
    check(broadphase.proxy_count()==2 && broadphase.pair_count()==1,
        "broadphase deduplicates entities, rejects invalid proxies, and includes touching AABBs");
    broadphase.add({.entity={2,0},.bounds={{4,0,0},{5,1,1}}});
    broadphase.build_pairs();
    check(broadphase.proxy_count()==2 && broadphase.pair_count()==0,"updating an existing proxy after sorting replaces its old bounds");
    broadphase.add({.entity={1,0},.bounds={{1,0,0},{0,1,1}}});
    broadphase.add({.entity={2,0},.bounds={{0,0,0},{1,1,1}}});
    broadphase.add({.entity={3,0},.bounds={{0,0,0},{1,1,1}},.dynamic=true});
    broadphase.build_pairs();
    check(broadphase.proxy_count()==2 && broadphase.pair_count()==1,"invalid replacement removes the proxy without corrupting other entity indices");
    broadphase.clear();
    check(broadphase.proxy_count()==0 && broadphase.pair_count()==0,"broadphase clear removes old frame state");

    world w;
    physics_world_3d simulation;
    const auto wall=body(w,{0,0,0},false);
    const auto cube=body(w,{0.75f,0,0});
    simulation.step(w,1.0f/120,10);
    check(simulation.events().size()==1 && has_event(simulation,collision_event_type_3d::enter,wall,cube,false),
        "solid contact emits enter once despite solver iterations");
    check(!simulation.events().empty() && simulation.events().front().fixed_tick_index==10,"collision events carry fixed tick index");
    check(near(w.get_component<transform_3d_component>(cube).position.x,1),"world resolves solid penetration");
    simulation.step(w,1.0f/120,11);
    check(simulation.events().size()==2 && has_event(simulation,collision_event_type_3d::stay,wall,cube,false),
        "resting exact-touch contact stays and events accumulate until consumed");
    if (!simulation.events().empty()) {
        const auto& event=simulation.events().back();
        check(event.first==wall && event.second==cube && event.normal.x< -0.9f,
            "event IDs are canonical and normal points from second to first");
    }
    simulation.clear_events();
    check(simulation.events().empty(),"clear_events only clears the delivery queue");
    w.get_component<transform_3d_component>(cube).position.x=3;
    simulation.step(w,1.0f/120,12);
    check(simulation.events().size()==1 && has_event(simulation,collision_event_type_3d::exit,wall,cube,false),
        "separation emits a single exit");
    simulation.clear_events();simulation.step(w,1.0f/120,13);
    check(simulation.events().empty(),"separated pairs do not repeat exit events");
    check(simulation.stats().collider_count==2 && simulation.stats().broadphase_pair_count==0 &&
        simulation.stats().narrowphase_test_count==0,"stats reset each step and omit separated narrowphase work");

    world sensors;
    physics_world_3d sensor_simulation;
    const auto sensor=body(sensors,{0,0,0},false,true);
    const auto visitor=body(sensors,{0.75f,0,0});
    sensor_simulation.step(sensors,1.0f/120,0);
    check(has_event(sensor_simulation,collision_event_type_3d::enter,sensor,visitor,true) &&
        near(sensors.get_component<transform_3d_component>(visitor).position.x,0.75f),
        "trigger detects overlap without applying a contact impulse or correction");
    sensor_simulation.clear_events();
    sensors.remove_tag(sensor,tag_id{"physics.trigger"});
    sensor_simulation.step(sensors,1.0f/120,1);
    check(sensor_simulation.events().size()==2 &&
        has_event(sensor_simulation,collision_event_type_3d::exit,sensor,visitor,true) &&
        has_event(sensor_simulation,collision_event_type_3d::enter,sensor,visitor,false),
        "trigger to solid transition exits old contact type and enters the new type");
    sensors.add_tag(sensor,tag_id{"physics.trigger"});sensor_simulation.clear_events();
    sensor_simulation.step(sensors,1.0f/120,2);
    check(has_event(sensor_simulation,collision_event_type_3d::exit,sensor,visitor,false) &&
        has_event(sensor_simulation,collision_event_type_3d::enter,sensor,visitor,true),
        "solid to trigger transition has a balanced lifecycle");
    sensor_simulation.clear_events();
    sensors.get_component<collision_filter_3d_component>(visitor).mask=0;
    sensor_simulation.step(sensors,1.0f/120,3);
    check(has_event(sensor_simulation,collision_event_type_3d::exit,sensor,visitor,true),
        "changing collision mask exits an active pair");
    sensors.get_component<collision_filter_3d_component>(visitor).mask=collision_layer_3d::all;
    sensor_simulation.clear_events();sensor_simulation.step(sensors,1.0f/120,4);
    check(has_event(sensor_simulation,collision_event_type_3d::enter,sensor,visitor,true),
        "restoring collision mask re-enters pair");
    const auto parent=sensors.create_entity();sensors.set_parent(visitor,parent);
    sensors.add_tag(parent,tag_id{"physics.disabled"});
    sensors.get_component<velocity_3d_component>(visitor).linear={1,0,0};
    const auto stopped=sensors.get_component<transform_3d_component>(visitor).position;
    sensor_simulation.clear_events();sensor_simulation.step(sensors,1.0f/120,5);
    check(has_event(sensor_simulation,collision_event_type_3d::exit,sensor,visitor,true) &&
        near(sensors.get_component<transform_3d_component>(visitor).position.x,stopped.x),
        "inherited physics.disabled stops integration and exits contacts");
    sensors.remove_tag(parent,tag_id{"physics.disabled"});
    sensors.get_component<velocity_3d_component>(visitor).linear={};
    sensor_simulation.clear_events();sensor_simulation.step(sensors,1.0f/120,6);
    check(has_event(sensor_simulation,collision_event_type_3d::enter,sensor,visitor,true),
        "removing disabled tag restores contact detection");
    sensors.set_entity_active(visitor,false);
    sensor_simulation.clear_events();sensor_simulation.step(sensors,1.0f/120,7);
    check(has_event(sensor_simulation,collision_event_type_3d::exit,sensor,visitor,true),
        "inactive entities exit contact tracking");
    sensors.set_entity_active(visitor,true);
    sensor_simulation.clear_events();sensor_simulation.step(sensors,1.0f/120,8);
    sensors.destroy_entity(visitor);
    const auto replacement=body(sensors,{0.75f,0,0});
    sensor_simulation.clear_events();sensor_simulation.step(sensors,1.0f/120,9);
    check(replacement.index==visitor.index && replacement.generation!=visitor.generation,
        "event lifecycle test recycles an entity index with a new generation");
    check(has_event(sensor_simulation,collision_event_type_3d::exit,sensor,visitor,true) &&
        has_event(sensor_simulation,collision_event_type_3d::enter,sensor,replacement,true),
        "destroyed entity exits without aliasing a recycled entity contact");
    sensors.remove_component<collider_3d_component>(replacement);
    sensor_simulation.clear_events();sensor_simulation.step(sensors,1.0f/120,10);
    check(has_event(sensor_simulation,collision_event_type_3d::exit,sensor,replacement,true),
        "removing collider exits the pair");
    sensors.add_component<collider_3d_component>(replacement);
    sensor_simulation.reset();sensor_simulation.step(sensors,1.0f/120,11);
    check(sensor_simulation.events().size()==1 && has_event(sensor_simulation,collision_event_type_3d::enter,sensor,replacement,true),
        "reset clears persistent contacts and starts with enter");
    sensor_simulation.clear_events();
    sensor_simulation.step(sensors,0,12);
    sensor_simulation.step(sensors,-1,12);
    sensor_simulation.step(sensors,std::numeric_limits<f32>::quiet_NaN(),12);
    check(sensor_simulation.events().empty(),"invalid world step does not emit lifecycle changes");
    sensor_simulation.step(sensors,1.0f/120,12);
    check(has_event(sensor_simulation,collision_event_type_3d::stay,sensor,replacement,true),
        "invalid steps preserve active contact cache");

    world static_sensors;
    physics_world_3d static_simulation;
    const auto static_sensor=body(static_sensors,{0,0,0},false,true);
    const auto static_target=body(static_sensors,{0.5f,0,0},false);
    static_simulation.step(static_sensors,1.0f/120,0);
    check(has_event(static_simulation,collision_event_type_3d::enter,static_sensor,static_target,true),
        "static triggers can observe static overlaps");
    world unsolved;
    physics_world_3d detect_only(physics_settings_3d{.solver_iterations=0});
    const auto detect_wall=body(unsolved,{0,0,0},false);
    const auto detect_cube=body(unsolved,{0.75f,0,0});
    detect_only.step(unsolved,1.0f/120,0);
    check(has_event(detect_only,collision_event_type_3d::enter,detect_wall,detect_cube,false) &&
        near(unsolved.get_component<transform_3d_component>(detect_cube).position.x,0.75f),
        "zero solver iterations still detects contacts without position response");
    world sparse;
    physics_world_3d sparse_simulation;
    for (u32 i=0;i<128;++i) body(sparse,{static_cast<f32>(i)*4,0,0});
    sparse_simulation.step(sparse,1.0f/120,0);
    check(sparse_simulation.stats().collider_count==128 && sparse_simulation.stats().broadphase_pair_count==0 &&
        sparse_simulation.stats().narrowphase_test_count==0,"128 separated colliders avoid all-pairs narrowphase tests");

    // Verify the production frame runner, not a second sample-only accumulator.
    system_scheduler scheduler;
    std::vector<tick_record> ticks;
    for (auto phase : {system_phase::input,system_phase::movement,system_phase::physics,
                       system_phase::post_physics,system_phase::post_update,system_phase::render_prepare})
        scheduler.add_system({.system_name=to_string(phase),.owner_name="sample.test",
            .phase=phase,.entity_query={},.function=record_tick,.user_data=&ticks});
    world frame_world;event_system events;
    fixed_step_settings settings;fixed_step_state state;
    scheduler.run_frame(frame_world,events,1.0f/60,7,settings,state);
    check(ticks.size()==8 && state.tick_index==2,"frame runner executes two physics steps per 60 Hz frame at 120 Hz");
    if (ticks.size()==8) {
        check(ticks[2].type==system_tick_type::fixed && ticks[2].tick==0 && ticks[2].frame==7 &&
            near(ticks[2].dt,settings.delta_seconds) && ticks[4].tick==1,
            "fixed phases receive fixed delta, tick type, frame index and monotonically increasing tick");
        check(ticks[3].phase==system_phase::post_physics && ticks[5].phase==system_phase::post_physics &&
            ticks[6].phase==system_phase::post_update && ticks[7].phase==system_phase::render_prepare,
            "post_physics runs per substep before hierarchy and render preparation");
    }
    ticks.clear();scheduler.run_frame(frame_world,events,1.0f/240,8,settings,state);
    check(ticks.size()==4 && near(state.interpolation_alpha,0.5f) && near(ticks.back().alpha,0.5f),
        "sub-frame remainder reaches post-update interpolation without a physics step");
    ticks.clear();
    scheduler.run_phase(system_phase::physics,frame_world,events,system_run_info{
        .delta_seconds=0.02f,.interpolation_alpha=0.25f,.frame_index=40,.fixed_tick_index=99,.tick_type=system_tick_type::fixed});
    check(ticks.size()==1 && ticks[0].tick==99 && ticks[0].frame==40 && near(ticks[0].dt,0.02f) && near(ticks[0].alpha,0.25f),
        "run_phase forwards all system_run_info fields");

    ticks.clear();
    fixed_step_state limited_state;
    auto limited=settings;limited.max_substeps=2;
    scheduler.run_frame(frame_world,events,0.25f,9,limited,limited_state);
    check(limited_state.tick_index==2 && limited_state.interpolation_alpha>=0 && limited_state.interpolation_alpha<1,
        "substep cap bounds catch-up work and keeps interpolation below one");
    ticks.clear();
    scheduler.run_frame(frame_world,events,0,10,limited,limited_state);
    check(limited_state.tick_index==2,"excess whole steps are discarded after the substep cap");
    fixed_step_state invalid_state;
    ticks.clear();scheduler.run_frame(frame_world,events,std::numeric_limits<f32>::quiet_NaN(),11,settings,invalid_state);
    check(invalid_state.tick_index==0 && ticks.size()==4 && near(ticks.front().dt,0),
        "invalid frame delta is sanitized without stepping physics");
    auto paused=settings;paused.max_substeps=0;
    ticks.clear();scheduler.run_frame(frame_world,events,1.0f/60,12,paused,invalid_state);
    check(invalid_state.tick_index==0 && ticks.size()==4,"zero substep budget leaves variable phases running");

    world demo;event_system demo_events;system_scheduler demo_scheduler;physics_demo_systems demo_physics;
    const auto handles=build_sample_3d_world(demo,{});
    register_sample_3d_systems(demo_scheduler,demo_physics,nullptr);
    for (u64 frame=0;frame<240;++frame) demo_physics.run_test_frame(demo_scheduler,demo,demo_events,1.0f/60,frame);
    check(near(demo.get_component<transform_3d_component>(handles.filtered_cube).position.y,0.5f,0.003f),
        "sample filtered cube crosses excluded platform and lands on world layer");
    auto& filter=demo.get_component<collision_filter_3d_component>(handles.filtered_cube);
    filter.mask|=collision_layer_3d::enemy;
    demo.get_component<transform_3d_component>(handles.filtered_cube).position={-6,5,2};
    demo.get_component<velocity_3d_component>(handles.filtered_cube).linear={};
    for (u64 frame=240;frame<480;++frame) demo_physics.run_test_frame(demo_scheduler,demo,demo_events,1.0f/60,frame);
    check(near(demo.get_component<transform_3d_component>(handles.filtered_cube).position.y,2.62f,0.003f),
        "sample mask toggle enables platform collision");
    demo.get_component<transform_3d_component>(handles.sphere).position={4,2,-1};
    demo.get_component<velocity_3d_component>(handles.sphere).linear={};
    demo_physics.run_test_frame(demo_scheduler,demo,demo_events,1.0f/120,480);
    check(demo.has_tag(handles.sphere,tag_id{"state.in_trigger"}) &&
        near(demo.get_component<mesh_3d_component>(handles.sphere).tint.y,1),
        "sample trigger event adds state.in_trigger and changes render tint");
    const auto sensor2=body(demo,{4,2,-1},false,true);
    demo_physics.run_test_frame(demo_scheduler,demo,demo_events,1.0f/120,481);
    check(demo.tag_count(handles.sphere,tag_id{"state.in_trigger"})==2,"overlapping triggers own separate tag references");
    demo.destroy_entity(sensor2);
    demo_physics.run_test_frame(demo_scheduler,demo,demo_events,1.0f/120,482);
    check(demo.tag_count(handles.sphere,tag_id{"state.in_trigger"})==1,
        "destroying one sensor retains the other sensor's tag reference");
    demo_physics.reset(demo);
    reset_sample_3d_world(demo,handles);
    check(!demo.has_tag(handles.sphere,tag_id{"state.in_trigger"}) &&
        near(demo.get_component<mesh_3d_component>(handles.sphere).tint.y,0.4f),
        "scene reset clears event-owned tags, restores tint and resets world cache");
    check(demo_physics.simulation.events().empty() && demo_physics.simulation.stats().collider_count==0,
        "sample reset clears physics statistics and pending events");

    std::printf("Physics World/Broadphase/events: %d checks, %d failures\n",result.checks,result.failures);
    return result;
}
}
