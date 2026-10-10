#include "sample_spatial_query.hpp"
#include "sample_physics_3d.hpp"
#include <rain/render/picking_3d.hpp>
#include <rain/render/mesh_3d_component.hpp>
#include <rain/runtime/local_matrix_3d_component.hpp>
#include <rain/runtime/rigid_body_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/velocity_3d_component.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <random>

namespace sample_3d {
namespace {
using namespace rain;
struct tests {
    int checks=0,failures=0;
    void check(bool condition,const char* name) {
        ++checks;if (!condition) { ++failures;std::fprintf(stderr,"FAIL: %s\n",name); }
    }
};
bool near(f32 a,f32 b,f32 tolerance=0.0002f) { return std::abs(a-b)<=tolerance; }
bool near(vec3 a,vec3 b,f32 tolerance=0.0002f) {
    return near(a.x,b.x,tolerance) && near(a.y,b.y,tolerance) && near(a.z,b.z,tolerance);
}
entity_id collider(world& w,vec3 position,collider_shape_3d shape=collider_shape_3d::box,
    vec3 half={1,1,1},f32 radius=1) {
    const auto e=w.create_entity();
    w.add_component<transform_3d_component>(e,transform_3d_component{.position=position});
    w.add_component<collider_3d_component>(e,collider_3d_component{.shape=shape,.half_extents=half,.radius=radius});
    return e;
}
void ray_and_filter_tests(tests& t) {
    world w;spatial_query_3d scene;
    const auto box=collider(w,{0,0,5});
    const auto sphere=collider(w,{0,0,10},collider_shape_3d::sphere);
    scene.sync(w);
    const ray_3d ray{{0,0,0},{0,0,8},20};
    auto hit=scene.raycast(ray);
    t.check(hit && hit->entity==box && near(hit->distance,4) && near(hit->fraction,0.2f),"ray chooses closest using world-unit distances");
    t.check(hit && near(hit->point,{0,0,4}) && near(hit->normal,{0,0,-1}) && !hit->started_overlapping,"ray point and outward normal");
    std::vector<spatial_query_hit_3d> all;
    scene.raycast_all(ray,all);
    t.check(all.size()==2 && all[0].entity==box && all[1].entity==sphere && near(all[1].distance,9),"all hits sorted nearest first");
    t.check(scene.raycast_any(ray),"any hit");
    t.check(!scene.raycast({{0,0,0},{0,0,-1},20}),"backward ray misses");
    t.check(!scene.raycast({{2,0,0},{0,0,1},20}),"parallel ray outside slab misses");
    t.check(!scene.raycast({{0,0,0},{0,0,1},3.99f}),"max distance clips before hit");
    t.check(scene.raycast({{0,0,0},{0,0,1},4}).has_value(),"endpoint contact included");
    t.check(scene.raycast({{1,0,0},{0,0,1},20}).has_value(),"ray along box face included");
    hit=scene.raycast({{0,0,5},{1,0,0},2});
    t.check(hit && hit->started_overlapping && hit->distance==0 && near(length(hit->normal),1),"inside ray returns initial contact");
    hit=scene.raycast({{0,0,5},{},0});
    t.check(hit && hit->entity==box && hit->fraction==0,"zero travel accepts zero direction");
    spatial_query_filter_3d filter;
    filter.ignored_entities={box};
    hit=scene.raycast(ray,filter);
    t.check(hit && hit->entity==sphere,"ignored entity does not block farther hit");
    w.add_tag(box,tag_id{"physics.trigger"});
    scene.sync(w);
    t.check(scene.raycast(ray)->entity==sphere,"triggers excluded by default");
    filter={};filter.triggers=query_trigger_mode_3d::include;
    t.check(scene.raycast(ray,filter)->entity==box && scene.raycast(ray,filter)->trigger,"include trigger");
    filter.triggers=query_trigger_mode_3d::only;
    scene.raycast_all(ray,all,filter);
    t.check(all.size()==1 && all[0].entity==box,"only trigger");
    w.add_component<collision_filter_3d_component>(box,collision_filter_3d_component{
        .layer=collision_layer_3d::enemy,.mask=collision_layer_3d::player});
    scene.sync(w);
    filter.layer_mask=collision_layer_3d::world;
    t.check(!scene.raycast(ray,filter),"query layer mask");
    filter.layer_mask=collision_layer_3d::enemy;
    t.check(scene.raycast(ray,filter).has_value(),"query ignores target simulation mask by default");
    filter.source_layer=collision_layer_3d::projectile;
    t.check(!scene.raycast(ray,filter),"optional bilateral mask rejects");
    filter.source_layer=collision_layer_3d::player;
    t.check(scene.raycast(ray,filter).has_value(),"optional bilateral mask accepts");
    w.add_tag(box,tag_id{"target.selectable"});w.add_tag(box,tag_id{"team.red"});scene.sync(w);
    filter.tags.require_all(tag_id{"target.selectable"}).require_any(tag_id{"team.red"}).require_any(tag_id{"team.blue"});
    t.check(scene.raycast(ray,filter).has_value(),"all and any Tag conditions");
    filter.tags.reject(tag_id{"team.red"});
    t.check(!scene.raycast(ray,filter),"none Tag condition");
    filter.tags.clear();filter.tags.require_all(tag_id{"target"});
    t.check(!scene.raycast(ray,filter),"Tag dotted names do not imply prefix matching");
    w.add_tag(sphere,tag_id{"physics.disabled"});w.set_entity_active(box,false);scene.sync(w);
    t.check(scene.collider_count()==0 && !scene.raycast_any(ray),"disabled and inactive excluded");
    scene.raycast_all(ray,all);
    t.check(all.empty(),"miss clears reused output");
    world tied;
    for (int i=0;i<17;++i) collider(tied,{0,0,5});
    scene.sync(tied);scene.raycast_all(ray,all);
    bool stable=all.size()==17;
    for (usize i=1;i<all.size();++i) stable &= all[i-1].entity.index<all[i].entity.index;
    t.check(stable && scene.raycast(ray)->entity==all.front().entity,"BVH ties ordered by full entity id");
    t.check(scene.node_count()>1,"nontrivial query tree constructed");
}
void sweep_and_overlap_tests(tests& t) {
    world w;spatial_query_3d scene;
    auto target=collider(w,{0,0,5});scene.sync(w);
    auto hit=scene.sweep_sphere({{0,0,0},0.5f},{0,0,7},20);
    t.check(hit && near(hit->distance,3.5f) && near(hit->point,{0,0,4}) && near(hit->normal,{0,0,-1}),"sphere sweep hits box face at analytic time");
    hit=scene.sweep_box({{0,0,0},{0.5f,0.5f,0.5f}},{0,0,1},20);
    t.check(hit && near(hit->distance,3.5f) && near(hit->point,{0,0,4}),"box sweep hits box at analytic time");
    auto& transform=w.get_component<transform_3d_component>(target);transform.position={};scene.sync(w);
    t.check(!scene.sweep_sphere({{-5,1.45f,1.45f},0.5f},{1,0,0},10),"rounded corner rejects expanded-AABB false positive");
    hit=scene.sweep_sphere({{-5,1.3f,1.3f},0.5f},{1,0,0},10);
    t.check(hit && near(hit->distance,4-std::sqrt(0.25f-0.18f)) && near(hit->point,{-1,1,1}),"sphere corner hit matches analytic root");
    t.check(hit && near(length(hit->normal),1) && hit->normal.x<0 && hit->normal.y>0,"corner normal is geometric");
    hit=scene.sweep_sphere({{-5,1.5f,0},0.5f},{1,0,0},10);
    t.check(hit && near(hit->distance,4) && near(hit->normal,{0,1,0}),"tangent sphere-edge contact");
    t.check(!scene.sweep_sphere({{-5,1.501f,0},0.5f},{1,0,0},10),"near tangent miss");
    hit=scene.sweep_sphere({{0,0,0},0.5f},{1,0,0},10);
    t.check(hit && hit->started_overlapping && hit->distance==0 && near(length(hit->normal),1),"initial sphere-box penetration is explicit");
    std::vector<spatial_query_hit_3d> hits;
    scene.overlap_sphere({{1.3f,1.3f,0},0.5f},hits);
    t.check(hits.size()==1 && hits[0].entity==target,"sphere overlap edge");
    scene.overlap_sphere({{1.45f,1.45f,0},0.5f},hits);
    t.check(hits.empty(),"overlap rejects diagonal false positive");
    scene.overlap_box({{2,0,0},{1,1,1}},hits);
    t.check(hits.size()==1 && hits[0].started_overlapping,"box overlap touching inclusive");
    scene.overlap_box({{2.001f,0,0},{1,1,1}},hits);
    t.check(hits.empty(),"separated box overlap misses");
    auto& shape=w.get_component<collider_3d_component>(target);shape.shape=collider_shape_3d::sphere;scene.sync(w);
    hit=scene.sweep_sphere({{-5,0,0},0.5f},{1,0,0},10);
    t.check(hit && near(hit->distance,3.5f) && near(hit->point,{-1,0,0}),"sphere-sphere sweep and target witness");
    hit=scene.sweep_box({{-5,0,0},{0.5f,0.5f,0.5f}},{1,0,0},10);
    t.check(hit && near(hit->distance,3.5f) && near(hit->normal,{-1,0,0}) && near(hit->point,{-1,0,0}),"box-sphere relative cast reverses normal correctly");
    hit=scene.sweep_box({{-5,1.3f,1.3f},{0.5f,0.5f,0.5f}},{1,0,0},10);
    t.check(!hit,"box-sphere rounded corner miss");
    hit=scene.sweep_box({{-5,1.1f,1.1f},{0.5f,0.5f,0.5f}},{1,0,0},10);
    t.check(hit && near(hit->distance,4.5f-std::sqrt(0.28f)),"box-sphere corner root");
    hit=scene.raycast({{-5,1,0},{1,0,0},10});
    t.check(hit && near(hit->distance,5) && near(hit->normal,{0,1,0}),"ray-sphere tangent");
    shape.shape=collider_shape_3d::box;shape.half_extents={0.001f,1,1};scene.sync(w);
    hit=scene.sweep_sphere({{-1000,0,0},0.01f},{1,0,0},2000);
    t.check(hit && near(hit->distance,999.989f,0.0003f),"long sweep does not tunnel through thin box");
    shape.half_extents={};scene.sync(w);
    hit=scene.raycast({{-5,0,0},{1,0,0},10});
    t.check(hit && near(hit->distance,5),"zero-size target point is supported");
    world many;
    collider(many,{0,0,5});collider(many,{0,0,10},collider_shape_3d::sphere);scene.sync(many);
    scene.sweep_sphere_all({{},0.5f},{0,0,1},20,hits);
    t.check(hits.size()==2 && near(hits[0].distance,3.5f) && near(hits[1].distance,8.5f),"sphere all-hit sweep sorted");
    scene.sweep_box_all({{},{0.5f,0.5f,0.5f}},{0,0,1},20,hits);
    t.check(hits.size()==2 && near(hits[0].distance,3.5f) && near(hits[1].distance,8.5f),"box all-hit sweep sorted");
    t.check(scene.sweep_sphere_any({{},0.5f},{0,0,1},20) && !scene.sweep_sphere_any({{},0.5f},{0,0,-1},20),"sphere any sweep hit and miss");
    t.check(scene.sweep_box_any({{},{0.5f,0.5f,0.5f}},{0,0,1},20) && !scene.sweep_box_any({{},{0.5f,0.5f,0.5f}},{0,0,-1},20),"box any sweep hit and miss");
    t.check(scene.overlap_sphere_any({{0,0,5},0.5f}) && !scene.overlap_sphere_any({{},0.5f}),"sphere any overlap hit and miss");
    t.check(scene.overlap_box_any({{0,0,5},{0.5f,0.5f,0.5f}}) && !scene.overlap_box_any({{},{0.5f,0.5f,0.5f}}),"box any overlap hit and miss");
}
void snapshot_and_input_tests(tests& t) {
    world w;physics_world_3d physics;
    auto target=collider(w,{0,0,5});physics.sync_queries(w);
    const ray_3d ray{{},{0,0,1},30};
    const auto revision=physics.queries().revision();
    w.get_component<transform_3d_component>(target).position.z=10;
    t.check(near(physics.queries().raycast(ray)->distance,4),"snapshot stays stable until explicit sync");
    physics.sync_queries(w);
    t.check(near(physics.queries().raycast(ray)->distance,9) && physics.queries().revision()>revision,"sync captures local edits without simulation");
    const auto parent=w.create_entity();
    w.add_component<transform_3d_component>(parent,transform_3d_component{.position={0,0,5},.scale={2,2,2}});
    w.get_component<transform_3d_component>(target).position={0,0,2};w.set_parent(target,parent);
    w.add_tag(target,tag_id{"transform.inherit_parent"});physics.sync_queries(w);
    auto hit=physics.queries().raycast(ray);
    t.check(hit && near(hit->distance,7),"parent scale and translation use fresh hierarchy");
    w.add_tag(parent,tag_id{"physics.disabled"});physics.sync_queries(w);
    t.check(!physics.queries().raycast(ray),"inherited physics.disabled excludes child");
    w.remove_tag(parent,tag_id{"physics.disabled"});w.add_tag(parent,tag_id{"team.red"});physics.sync_queries(w);
    spatial_query_filter_3d filter;filter.tags.require_all(tag_id{"team.red"});
    t.check(!physics.queries().raycast(ray,filter),"query Tags remain local, not entity inheritance");
    w.add_component<local_matrix_3d_component>(target,local_matrix_3d_component{.matrix=make_translation({0,0,4})});physics.sync_queries(w);
    t.check(near(physics.queries().raycast(ray)->distance,11),"local matrix override shared with simulation");
    w.destroy_entity(target);
    const auto recycled=collider(w,{0,0,6});
    t.check(recycled.index==target.index && recycled.generation!=target.generation,"generation reuse test setup");
    physics.sync_queries(w);filter={};filter.ignored_entities={target};
    hit=physics.queries().raycast(ray,filter);
    t.check(hit && hit->entity==recycled,"old ignored id does not exclude recycled entity");
    physics.reset();t.check(physics.queries().collider_count()==0 && !physics.queries().raycast(ray),"reset clears query snapshot");
    const f32 nan=std::numeric_limits<f32>::quiet_NaN(),inf=std::numeric_limits<f32>::infinity();
    physics.sync_queries(w);
    const auto& scene=physics.queries();
    t.check(!scene.raycast({{},{},1}),"zero direction with positive travel rejected");
    t.check(!scene.raycast({{},{0,0,1},-1}),"negative distance rejected");
    t.check(!scene.raycast({{nan,0,0},{0,0,1},10}) && !scene.raycast({{},{inf,0,0},10}),"nonfinite origin and direction rejected");
    t.check(!scene.raycast({{},{0,0,1},inf}),"infinite range rejected explicitly");
    t.check(!scene.sweep_sphere({{},-1},{0,0,1},10) && !scene.sweep_sphere({{},nan},{0,0,1},10),"invalid query radius rejected");
    t.check(!scene.sweep_box({{},{-1,1,1}},{0,0,1},10),"negative query half extents rejected");
    t.check(scene.raycast({{},{0,0,std::numeric_limits<f32>::max()},10}).has_value(),"huge finite direction normalized safely");
    t.check(scene.raycast({{},{0,0,1e-20f},10}).has_value(),"tiny nonzero direction normalized safely");
    std::vector<spatial_query_hit_3d> hits(1);
    scene.sweep_box_all({{},{nan,1,1}},{0,0,1},10,hits);t.check(hits.empty(),"invalid request clears output");
    const auto invalid=collider(w,{inf,0,0});
    w.get_component<collider_3d_component>(invalid).radius=nan;physics.sync_queries(w);
    t.check(physics.queries().collider_count()==1,"nonfinite collider excluded from BVH");
    // Test post-solver sync: a dynamic body is moved out of a static overlap.
    world simulated;const auto floor=collider(simulated,{0,-1,0});
    const auto body=collider(simulated,{0,0.25f,0},collider_shape_3d::box,{0.5f,0.5f,0.5f});
    simulated.add_tag(body,tag_id{"physics.dynamic"});
    simulated.add_component<rigid_body_3d_component>(body);
    simulated.add_component<velocity_3d_component>(body);
    physics.reset();physics.step(simulated,1.0f/120,0);
    filter={};filter.ignored_entities={floor};
    hit=physics.queries().raycast({{0,5,0},{0,-1,0},10},filter);
    t.check(hit && near(hit->distance,4),"physics step publishes post-solver transforms");
    const auto events=physics.events().size();const auto position=simulated.get_component<transform_3d_component>(body).position;
    for (int i=0;i<20;++i) (void)physics.queries().sweep_box({{0,5,0},{0.5f,0.5f,0.5f}},{0,-1,0},10);
    t.check(physics.events().size()==events && near(simulated.get_component<transform_3d_component>(body).position,position),"queries do not simulate, move entities or emit collision events");
    // Same scheduler as the app, with no fixed substep this frame.
    world scheduled;event_system events_system;system_scheduler scheduler;physics_demo_systems demo;
    auto e=collider(scheduled,{0,0,5});register_sample_3d_systems(scheduler,demo,nullptr);
    demo.run_test_frame(scheduler,scheduled,events_system,0,0);
    t.check(demo.step_count==0 && demo.simulation.queries().raycast(ray).has_value(),"registered query sync runs even on zero-substep frame");
    scheduled.get_component<transform_3d_component>(e).position.z=12;
    demo.run_test_frame(scheduler,scheduled,events_system,0,1);
    t.check(near(demo.simulation.queries().raycast(ray)->distance,11),"frame query sync catches editor changes without stepping");
}
void transformed_geometry_tests(tests& t) {
    world w;spatial_query_3d scene;
    const auto sphere=collider(w,{0,0,10},collider_shape_3d::sphere);
    auto& transform=w.get_component<transform_3d_component>(sphere);
    transform.scale={-2,1,0.5f};
    w.get_component<collider_3d_component>(sphere).center={1,0,0};scene.sync(w);
    auto hit=scene.raycast({{-2,0,0},{0,0,1},20});
    t.check(hit && near(hit->distance,8),"negative nonuniform scale and local sphere center");
    w.get_component<collider_3d_component>(sphere).shape=collider_shape_3d::box;
    w.get_component<collider_3d_component>(sphere).center={};
    transform.scale={1,1,1};transform.rotation={0,0.78539816339f,0};scene.sync(w);
    hit=scene.raycast({{0,0,0},{0,0,1},20});
    t.check(hit && near(hit->distance,10-std::sqrt(2.0f)),"rotated V0 box uses conservative world AABB");
    transform.rotation={};transform.scale={0,0,0};scene.sync(w);
    t.check(scene.raycast({{},{0,0,1},10}).has_value(),"zero-scale target remains a queryable point");
    // Sheared sphere scale must match the bound used by physics, not max axis length.
    auto matrix=mat4::identity();matrix.values[0][1]=1;matrix.values[3][2]=10;
    w.add_component<local_matrix_3d_component>(sphere,local_matrix_3d_component{.matrix=matrix});
    w.get_component<collider_3d_component>(sphere).shape=collider_shape_3d::sphere;scene.sync(w);
    hit=scene.raycast({{},{0,0,1},20});
    t.check(hit && near(hit->distance,10-std::sqrt(3.0f)),"sheared sphere uses shared conservative spectral bound");
    w.remove_component<local_matrix_3d_component>(sphere);
    auto& shape=w.get_component<collider_3d_component>(sphere);shape.shape=collider_shape_3d::box;shape.half_extents={0.1f,1,1};
    transform.position={10000,0,0};transform.scale={1,1,1};scene.sync(w);
    // 9.9f itself rounds below the true entry from 10000 - 0.1f. The next
    // representable distance reaches it while still preceding an inward-rounded BVH bound.
    t.check(!scene.raycast({{9990,0,0},{1,0,0},9.9f}),"strict distance endpoint does not extend a short ray");
    hit=scene.raycast({{9990,0,0},{1,0,0},std::nextafter(9.9f,10.0f)});
    t.check(hit && near(hit->distance,9.9f),"outward-rounded BVH retains large-coordinate boundary contact");
}
void picking_tests(tests& t) {
    camera_3d_frame camera;world_transform_3d_component transform;
    camera_3d_component component{.vertical_fov_radians=1.57079632679f,.near_plane=1,.far_plane=100};
    t.check(build_camera_frame_3d(transform,component,2,camera),"camera shared frame construction");
    ray_3d ray;
    t.check(screen_point_to_ray_3d(camera,{0,0,800,400},{400,200},ray) && near(ray.origin,{0,0,1}) && near(ray.direction,{0,0,1}) && near(ray.max_distance,99,0.001f),"center picking ray near/far clip segment");
    t.check(screen_point_to_ray_3d(camera,{50,80,800,400},{450,280},ray) && near(ray.direction,{0,0,1}),"viewport offset handled");
    t.check(screen_point_to_ray_3d(camera,{0,0,800,400},{0,0},ray) && near(ray.origin,{-2,1,1}),"top-left Y flip and aspect correct");
    const auto first=ray;
    t.check(screen_point_to_ray_3d(camera,{0,0,1600,800},{0,0},ray) && near(ray.origin,first.origin),"same normalized position independent of pixel scale");
    t.check(!screen_point_to_ray_3d(camera,{0,0,800,400},{800,200},ray) && ray.max_distance==0,"right edge outside and resets result");
    t.check(!screen_point_to_ray_3d(camera,{0,0,800,400},{200,-1},ray),"outside pointer rejected");
    t.check(!screen_point_to_ray_3d(camera,{0,0,0,400},{0,0},ray),"minimized viewport rejected");
    const auto valid=camera;
    camera.view_projection={};t.check(!screen_point_to_ray_3d(camera,{0,0,800,400},{400,200},ray),"singular matrix rejected");
    camera=valid;camera.view_projection.values[0][0]=std::numeric_limits<f32>::quiet_NaN();
    t.check(!screen_point_to_ray_3d(camera,{0,0,800,400},{400,200},ray),"nonfinite matrix rejected");
    transform.position={5,2,-3};transform.forward={1,0,0};
    t.check(build_camera_frame_3d(transform,component,2,camera) &&
        screen_point_to_ray_3d(camera,{0,0,800,400},{400,200},ray) && near(ray.origin,{6,2,-3}) && near(ray.direction,{1,0,0}),"rotated translated camera ray matches rendering basis");
    transform.up={1,0,0};t.check(!build_camera_frame_3d(transform,component,2,camera),"degenerate camera basis rejected");
    // Generic inverse also handles orthographic view_projection (D3D depth 0..1).
    camera.view_projection=mat4::identity();camera.view_projection.values[0][0]=0.5f;
    camera.view_projection.values[2][2]=0.1f;
    t.check(screen_point_to_ray_3d(camera,{0,0,800,400},{600,100},ray) && near(ray.origin,{1,0.5f,0}) && near(ray.direction,{0,0,1}) && near(ray.max_distance,10),"orthographic unprojection supported");
    world w;physics_world_3d physics;
    const auto entity=collider(w,{0,0,5});
    w.add_component<mesh_3d_component>(entity,mesh_3d_component{.mesh={},.material={},.tint={0.2f,0.3f,0.4f,1}});
    w.add_tag(entity,tag_id{"object.renderable"});physics.sync_queries(w);
    spatial_query_demo demo;demo.physics=&physics;demo.log_selection=false;demo.initialize(w,{});
    ray={{},{0,0,1},20};demo.evaluate(w,&ray,true);
    t.check(demo.selected==entity && w.tag_count(entity,tag_id{"state.selected"})==1 && demo.last_hit.has_value(),"sample click selects through shared query API");
    demo.evaluate(w,&ray,true);
    t.check(w.tag_count(entity,tag_id{"state.selected"})==1,"repeated click does not leak reference-counted selection Tag");
    demo.mode=spatial_demo_mode::sphere;demo.evaluate(w,&ray,false);
    t.check(demo.last_hit && near(demo.last_hit->distance,3.55f),"sample sphere preview uses sweep distance");
    demo.mode=spatial_demo_mode::box;demo.evaluate(w,&ray,false);
    t.check(demo.last_hit && near(demo.last_hit->distance,3.55f),"sample box preview uses sweep distance");
    physics.sync_queries(w);
    t.check(physics.queries().collider_count()==1,"visual markers never enter physics query scene");
    ray.direction={0,1,0};demo.evaluate(w,&ray,true);
    t.check(!demo.selected.is_valid() && !w.has_tag(entity,tag_id{"state.selected"}) && near(w.get_component<mesh_3d_component>(entity).tint.x,0.2f),"miss clears selection and restores original tint");
    ray.direction={0,0,1};demo.evaluate(w,&ray,true);demo.reset(w);
    t.check(!demo.selected.is_valid() && !w.has_tag(entity,tag_id{"state.selected"}),"reset clears demo-owned selection state");
    const auto parent=w.create_entity();w.set_parent(entity,parent);w.add_tag(parent,tag_id{"render.hidden"});
    physics.sync_queries(w);demo.evaluate(w,&ray,true);
    t.check(!demo.last_hit && !demo.selected.is_valid(),"hidden ancestor prevents picking");
    w.remove_tag(parent,tag_id{"render.hidden"});physics.sync_queries(w);demo.evaluate(w,&ray,true);
    w.remove_component<collider_3d_component>(entity);physics.sync_queries(w);demo.evaluate(w,nullptr,false);
    t.check(!demo.selected.is_valid() && !w.has_tag(entity,tag_id{"state.selected"}),"removing collider clears selection and owned Tag");
}
// Independent continuous oracle: convex squared-distance minimization, then
// bisection for first entry. It does not use slabs, BVH or piecewise polynomials.
std::optional<double> oracle(vec3 origin,vec3 direction,vec3 center,vec3 extent,double radius,double travel) {
    const auto distance=[&](double time) {
        const double x=std::max(std::abs(origin.x+direction.x*time-center.x)-extent.x,0.0);
        const double y=std::max(std::abs(origin.y+direction.y*time-center.y)-extent.y,0.0);
        const double z=std::max(std::abs(origin.z+direction.z*time-center.z)-extent.z,0.0);
        return x*x+y*y+z*z-radius*radius;
    };
    if (distance(0)<=0) return 0;
    double lo=0,hi=travel;
    for (int i=0;i<100;++i) {
        const double a=lo+(hi-lo)/3,b=hi-(hi-lo)/3;
        if (distance(a)<distance(b)) hi=b;else lo=a;
    }
    double inside=(lo+hi)*0.5;
    if (distance(inside)>1e-10) return std::nullopt;
    lo=0;hi=inside;
    for (int i=0;i<100;++i) { const double m=(lo+hi)*0.5;if (distance(m)<=1e-12) hi=m;else lo=m; }
    return hi;
}
void randomized_tests(tests& t) {
    std::mt19937 random(0x51a7u);
    std::uniform_real_distribution<f32> position(-4,4),size(0.1f,1.4f);
    for (int kind=0;kind<4;++kind) for (int iteration=0;iteration<100;++iteration) {
        world w;spatial_query_3d scene;
        const vec3 center{position(random),position(random),position(random)};
        const vec3 half{size(random),size(random),size(random)};
        const f32 target_radius=size(random),query_radius=size(random);
        const vec3 query_half{size(random),size(random),size(random)};
        const bool target_sphere=(kind&1)!=0,query_sphere=(kind&2)==0;
        collider(w,center,target_sphere ? collider_shape_3d::sphere : collider_shape_3d::box,half,target_radius);
        scene.sync(w);
        const vec3 origin{position(random),position(random),position(random)};
        const vec3 direction=normalize(vec3{position(random),position(random),position(random)});
        const auto actual=query_sphere ? scene.sweep_sphere({origin,query_radius},direction,12) :
            scene.sweep_box({origin,query_half},direction,12);
        std::optional<double> expected;
        if (query_sphere) expected=oracle(origin,direction,center,target_sphere ? vec3{} : half,
            query_radius+(target_sphere ? target_radius : 0),12);
        else if (target_sphere) expected=oracle(center,-direction,origin,query_half,target_radius,12);
        else expected=oracle(origin,direction,center,half+query_half,0,12);
        t.check(actual.has_value()==expected.has_value() && (!actual || near(actual->distance,static_cast<f32>(*expected),0.001f)),
            "randomized sweep versus independent convex-distance oracle");
    }
    // Cross-check tree pruning and all-hit order against isolated target scenes.
    world w;std::vector<vec3> centers;
    for (int i=0;i<128;++i) {
        centers.push_back({position(random)*3,position(random)*3,position(random)*3});
        collider(w,centers.back(),i%2==0 ? collider_shape_3d::box : collider_shape_3d::sphere);
    }
    spatial_query_3d tree;tree.sync(w);
    for (int trial=0;trial<30;++trial) {
        ray_3d ray{{position(random),position(random),-18},normalize(vec3{position(random)*0.1f,position(random)*0.1f,1}),40};
        std::vector<spatial_query_hit_3d> actual;tree.raycast_all(ray,actual);
        std::vector<std::pair<f32,u32>> expected;
        for (u32 i=0;i<centers.size();++i) {
            const auto hit=oracle(ray.origin,ray.direction,centers[i],i%2==0 ? vec3{1,1,1} : vec3{},i%2==0 ? 0 : 1,ray.max_distance);
            if (hit) expected.emplace_back(static_cast<f32>(*hit),i);
        }
        std::sort(expected.begin(),expected.end());
        bool matches=actual.size()==expected.size();
        if (matches) for (usize i=0;i<actual.size();++i)
            matches &= actual[i].entity.index==expected[i].second && near(actual[i].distance,expected[i].first,0.001f);
        t.check(matches,"BVH raycast all matches independent brute-force oracle");
    }
}
}
int run_spatial_query_tests() {
    tests t;
    ray_and_filter_tests(t);sweep_and_overlap_tests(t);snapshot_and_input_tests(t);transformed_geometry_tests(t);picking_tests(t);randomized_tests(t);
    std::printf("Spatial Query: %d checks, %d failures\n",t.checks,t.failures);
    return t.failures==0 ? 0 : 1;
}
}
