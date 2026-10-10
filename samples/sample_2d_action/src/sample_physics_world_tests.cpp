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

// Use a known gravity and zero damping so friction has an analytic oracle.
struct friction_test_scene {
    world target;
    physics_world_3d simulation;
    entity_id ground,cube;
    u64 tick=0;

    explicit friction_test_scene(u32 iterations=4,bool reverse_entity_order=false)
        : simulation(physics_settings_3d{.gravity={0,-10,0},.solver_iterations=iterations}) {
        if (reverse_entity_order) cube=body(target,{0,0.5f,0});
        ground=body(target,{0,-0.5f,0},false);
        if (!reverse_entity_order) cube=body(target,{0,0.5f,0});
        target.get_component<collider_3d_component>(ground).half_extents={50,0.5f,50};
        target.remove_tag(cube,tag_id{"physics.no_gravity"});
        target.get_component<velocity_3d_component>(cube).linear={2,0,0};
    }
    void material(entity_id entity,f32 static_friction,f32 dynamic_friction) {
        auto& rigid=target.get_component<rigid_body_3d_component>(entity);
        rigid.static_friction=static_friction;
        rigid.dynamic_friction=dynamic_friction;
    }
    void materials(f32 static_friction,f32 dynamic_friction) {
        material(ground,static_friction,dynamic_friction);
        material(cube,static_friction,dynamic_friction);
    }
    void step(u32 count=1) {
        for (u32 i=0;i<count;++i) {
            simulation.step(target,1.0f/120.0f,tick++);
            simulation.clear_events();
        }
    }
    vec3 velocity() const { return target.get_component<velocity_3d_component>(cube).linear; }
};

physics_sample_test_result run_friction_tests() {
    physics_sample_test_result result;
    const auto check=[&](bool condition,const char* name) {
        ++result.checks;
        if (!condition) { ++result.failures; std::fprintf(stderr,"FAIL: %s\n",name); }
    };
    const auto near=[](f32 a,f32 b) { return std::abs(a-b)<0.0002f; };
    friction_test_scene defaults;
    defaults.step(60);
    check(near(defaults.velocity().x,2),"zero default friction preserves existing tangential motion");
    friction_test_scene zero_surface;
    zero_surface.material(zero_surface.ground,1,1);
    zero_surface.step(60);
    check(near(zero_surface.velocity().x,2),"a frictionless surface keeps the mixed pair frictionless");
    friction_test_scene sliding;
    sliding.materials(0.5f,0.3f);
    sliding.step(60);
    // v = v0 - mu*g*t = 2 - 0.3*10*0.5 = 0.5, independently of solver iterations.
    check(near(sliding.velocity().x,0.5f),"kinetic friction matches analytic deceleration over half a second");
    check(near(sliding.target.get_component<transform_3d_component>(sliding.cube).position.y,0.5f),
        "friction preserves normal support on the floor");
    bool monotonic=true;
    for (u32 i=0;i<120;++i) {
        const f32 before=sliding.velocity().x;
        sliding.step();
        monotonic &= sliding.velocity().x>=-0.00001f && sliding.velocity().x<=before+0.00001f;
    }
    check(monotonic && near(sliding.velocity().x,0),"friction stops without reversing or reaccelerating a body");

    friction_test_scene static_capture;
    static_capture.materials(0.5f,0.1f);
    static_capture.target.get_component<velocity_3d_component>(static_capture.cube).linear.x=0.035f;
    static_capture.step();
    check(near(static_capture.velocity().x,0),"static friction captures motion below its normal-impulse budget");
    friction_test_scene breakaway;
    breakaway.materials(0.5f,0.1f);
    breakaway.target.get_component<velocity_3d_component>(breakaway.cube).linear.x=0.08f;
    breakaway.step();
    check(near(breakaway.velocity().x,0.08f-0.1f*10/120),"exceeding static capacity uses the kinetic coefficient");
    friction_test_scene mixed;
    mixed.material(mixed.ground,0.9f,0.9f);
    mixed.material(mixed.cube,0.1f,0.1f);
    mixed.step(60);
    check(near(mixed.velocity().x,0.5f),"different surface coefficients combine using the geometric mean");
    friction_test_scene bounded;
    bounded.materials(0.1f,10);
    bounded.step();
    check(near(bounded.velocity().x,2.0f-0.1f*10/120),"effective kinetic friction is bounded by static friction");
    bool invalid_safe=true;
    for (f32 invalid : {-1.0f,std::numeric_limits<f32>::quiet_NaN(),std::numeric_limits<f32>::infinity()}) {
        friction_test_scene bad;
        bad.material(bad.ground,1,1);
        bad.material(bad.cube,invalid,invalid);
        bad.step();
        invalid_safe &= near(bad.velocity().x,2) && std::isfinite(bad.velocity().y);
    }
    check(invalid_safe,"negative and nonfinite friction coefficients safely act as zero");
    friction_test_scene invalid_kinetic;
    invalid_kinetic.materials(0.5f,std::numeric_limits<f32>::quiet_NaN());
    invalid_kinetic.step();
    check(near(invalid_kinetic.velocity().x,2),"invalid kinetic coefficient does not poison sliding velocities");
    friction_test_scene large;
    large.materials(std::numeric_limits<f32>::max(),std::numeric_limits<f32>::max());
    large.step();
    check(near(large.velocity().x,0) && std::isfinite(large.velocity().y),"large finite coefficients do not overflow material mixing");

    friction_test_scene airborne;
    airborne.materials(1,1);
    airborne.target.get_component<transform_3d_component>(airborne.cube).position.y=5;
    airborne.step(60);
    check(near(airborne.velocity().x,2),"contact friction does not act as free-flight damping");
    friction_test_scene unloaded;
    unloaded.materials(1,1);
    unloaded.target.add_tag(unloaded.cube,tag_id{"physics.no_gravity"});
    unloaded.step(60);
    check(near(unloaded.velocity().x,2),"touching without a normal impulse does not invent friction load");
    friction_test_scene trigger;
    trigger.materials(1,1);
    trigger.target.add_tag(trigger.ground,tag_id{"physics.trigger"});
    trigger.step();
    check(near(trigger.velocity().x,2) && trigger.velocity().y<0,"triggers apply neither friction nor support");
    friction_test_scene disabled;
    disabled.materials(1,1);
    disabled.target.add_tag(disabled.ground,tag_id{"physics.disabled"});
    disabled.step();
    check(near(disabled.velocity().x,2) && disabled.velocity().y<0,"disabled colliders apply no friction");
    friction_test_scene detect_only(0);
    detect_only.materials(1,1);
    detect_only.step();
    check(near(detect_only.velocity().x,2) && detect_only.velocity().y<0,"zero solver iterations disables friction response");
    friction_test_scene collider_only;
    collider_only.material(collider_only.cube,1,1);
    collider_only.target.remove_component<rigid_body_3d_component>(collider_only.ground);
    collider_only.step();
    check(near(collider_only.velocity().x,2) && near(collider_only.velocity().y,0),
        "a collider without rigid-body material remains frictionless and still supports bodies");

    friction_test_scene dynamic_pair;
    dynamic_pair.materials(0.5f,0.3f);
    dynamic_pair.target.remove_tag(dynamic_pair.ground,tag_id{"physics.static"});
    dynamic_pair.target.add_tag(dynamic_pair.ground,tag_id{"physics.dynamic"});
    dynamic_pair.target.get_component<rigid_body_3d_component>(dynamic_pair.ground).inverse_mass=0.5f;
    dynamic_pair.target.add_tag(dynamic_pair.cube,tag_id{"physics.no_gravity"});
    dynamic_pair.target.get_component<velocity_3d_component>(dynamic_pair.cube).linear={2,-1,0};
    dynamic_pair.step();
    const auto lower_velocity=dynamic_pair.target.get_component<velocity_3d_component>(dynamic_pair.ground).linear;
    check(near(dynamic_pair.velocity().x+2*lower_velocity.x,2) && lower_velocity.x>0 && dynamic_pair.velocity().x<2,
        "friction transfers tangential momentum between unequal dynamic masses");
    friction_test_scene conveyor;
    conveyor.materials(1,1);
    conveyor.target.remove_tag(conveyor.ground,tag_id{"physics.static"});
    conveyor.target.add_tag(conveyor.ground,tag_id{"physics.kinematic"});
    conveyor.target.get_component<velocity_3d_component>(conveyor.ground).linear={1,0,0};
    conveyor.target.get_component<velocity_3d_component>(conveyor.cube).linear={};
    conveyor.step(30);
    check(near(conveyor.velocity().x,1) && near(conveyor.target.get_component<velocity_3d_component>(conveyor.ground).linear.x,1),
        "friction follows kinematic surface velocity without changing that surface's velocity");
    friction_test_scene sphere;
    sphere.materials(0.5f,0.3f);
    sphere.target.get_component<collider_3d_component>(sphere.cube).shape=collider_shape_3d::sphere;
    sphere.target.get_component<velocity_3d_component>(sphere.cube).linear={0,0,2};
    sphere.step(60);
    check(near(sphere.velocity().z,0.5f) && near(sphere.velocity().x,0),"sphere-box friction handles the second tangential axis");
    friction_test_scene diagonal;
    diagonal.materials(0.5f,0.3f);
    diagonal.target.get_component<velocity_3d_component>(diagonal.cube).linear={1.2f,0,1.6f};
    diagonal.step(60);
    check(near(diagonal.velocity().x,0.3f) && near(diagonal.velocity().z,0.4f),
        "diagonal sliding has one isotropic friction budget, not one per axis");
    friction_test_scene light;
    light.materials(0.5f,0.3f);
    light.target.get_component<rigid_body_3d_component>(light.cube).inverse_mass=2;
    light.step(60);
    check(near(light.velocity().x,0.5f),"supported sliding deceleration is independent of mass");
    friction_test_scene one_iteration(1),many_iterations(8),reversed(4,true);
    for (auto* scene : {&one_iteration,&many_iterations,&reversed}) {
        scene->materials(0.5f,0.3f);scene->step(60);
    }
    check(near(one_iteration.velocity().x,many_iterations.velocity().x) && near(many_iterations.velocity().x,0.5f),
        "solver iterations do not multiply single-contact friction");
    check(near(reversed.velocity().x,one_iteration.velocity().x),"friction is independent of canonical entity ordering");
    std::printf("Friction: %d checks, %d failures\n",result.checks,result.failures);
    return result;
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
    const auto& rough_position=demo.get_component<transform_3d_component>(handles.rough_cube).position;
    const auto& slippery_position=demo.get_component<transform_3d_component>(handles.slippery_cube).position;
    check(slippery_position.x>rough_position.x+2 && near(rough_position.y,0.4f) && near(slippery_position.y,0.4f) &&
        near(demo.get_component<velocity_3d_component>(handles.rough_cube).linear.x,0) &&
        near(demo.get_component<velocity_3d_component>(handles.slippery_cube).linear.x,0),
        "sample low-friction cube travels farther while both cubes settle on the same floor");
    const auto camera_before=demo.get_component<transform_3d_component>(handles.camera).position;
    restart_friction_demo(demo,handles);
    check(near(rough_position.x,-6) && near(slippery_position.x,-6) &&
        near(demo.get_component<velocity_3d_component>(handles.rough_cube).linear.x,3) &&
        near(demo.get_component<velocity_3d_component>(handles.slippery_cube).linear.x,3) &&
        near(demo.get_component<transform_3d_component>(handles.camera).position.z,camera_before.z),
        "friction replay resets identical initial conditions without moving the camera");
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

    const auto friction=run_friction_tests();
    result.checks+=friction.checks;
    result.failures+=friction.failures;
    std::printf("Physics World/Broadphase/events/friction: %d checks, %d failures\n",result.checks,result.failures);
    return result;
}
}
