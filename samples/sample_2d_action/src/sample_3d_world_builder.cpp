#include "sample_3d_world_builder.hpp"
#include "sample_physics_3d.hpp"

#include <rain/render/camera_3d.hpp>
#include <rain/render/directional_light_3d_component.hpp>
#include <rain/render/mesh_3d_component.hpp>
#include <rain/runtime/camera_input_3d_component.hpp>
#include <rain/runtime/collider_3d_component.hpp>
#include <rain/runtime/collision_filter_3d_component.hpp>
#include <rain/runtime/rigid_body_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/velocity_3d_component.hpp>

#include <string_view>

namespace sample_3d {
namespace {
using namespace rain;

entity_id create_body(world& w, const sample_3d_world_resources& resources,
    const char* name, vec3 position, vec3 scale, vec4 color, const char* mode,
    bool sphere = false) {
    const auto entity = w.create_entity({.name=string_id{name}});
    w.add_component<transform_3d_component>(entity,
        transform_3d_component{.position=position, .scale=scale});
    w.add_component<mesh_3d_component>(entity, mesh_3d_component{
        .mesh=sphere ? resources.sphere_mesh : resources.cube_mesh,
        .material=resources.cube_material, .tint=color});
    // Both procedural meshes have unit radius / half extent before transform scale.
    w.add_component<collider_3d_component>(entity, collider_3d_component{
        .shape=sphere ? collider_shape_3d::sphere : collider_shape_3d::box,
        .half_extents={1,1,1}, .radius=1});
    w.add_component<rigid_body_3d_component>(entity, rigid_body_3d_component{
        .inverse_mass=std::string_view{mode} == "physics.dynamic" ? 1.0f : 0.0f,
        .restitution=sphere ? 0.65f : 0.0f});
    w.add_component<velocity_3d_component>(entity);
    w.add_component<collision_filter_3d_component>(entity);
    w.add_tag(entity,tag_id{"physics.collider"});
    if (std::string_view{mode}!="physics.static")
        w.add_component<physics_feedback_component>(entity,physics_feedback_component{.normal_tint=color});
    for (const char* tag : {"transform.3d", "object.renderable", "render.3d",
                            "render.frustum_cull", "object.movable"})
        w.add_tag(entity, tag_id{tag});
    w.add_tag(entity, tag_id{mode});
    return entity;
}
}

sample_3d_world_handles build_sample_3d_world(world& w,
    const sample_3d_world_resources& resources) {
    sample_3d_world_handles result{};
    result.camera = w.create_entity({.name=string_id{"entity.camera_3d"}});
    w.add_component<transform_3d_component>(result.camera);
    w.add_component<camera_3d_component>(result.camera, camera_3d_component{.far_plane=150});
    w.add_component<camera_input_3d_component>(result.camera);
    w.add_component<velocity_3d_component>(result.camera);
    for (const char* tag : {"transform.3d", "camera.3d", "camera.active", "camera.input", "object.movable"})
        w.add_tag(result.camera, tag_id{tag});

    result.ground = create_body(w, resources, "physics.ground", {0,-0.5f,1}, {9,0.5f,6},
        {0.26f,0.31f,0.38f,1}, "physics.static");
    w.get_component<collision_filter_3d_component>(result.ground).layer=collision_layer_3d::world;
    result.cube = create_body(w, resources, "physics.cube", {-4,4,0}, {0.65f,0.65f,0.65f},
        {0.10f,0.52f,1,1}, "physics.dynamic");
    result.frozen_cube = create_body(w, resources, "physics.frozen", {-1.5f,3,0}, {0.6f,0.6f,0.6f},
        {1,0.73f,0.10f,1}, "physics.dynamic");
    result.floating_cube = create_body(w, resources, "physics.floating", {1,3,2.5f}, {0.6f,0.6f,0.6f},
        {0.7f,0.25f,1,1}, "physics.dynamic");
    result.kinematic_cube = create_body(w, resources, "physics.kinematic", {-3,0.5f,3}, {0.8f,0.5f,0.8f},
        {0.15f,0.9f,0.4f,1}, "physics.kinematic");
    w.add_tag(result.kinematic_cube,tag_id{"sample.kinematic_patrol"});
    result.trigger_platform = create_body(w, resources, "physics.trigger_platform", {4,2,-1}, {1.2f,0.12f,1.2f},
        {0.9f,0.2f,0.32f,1}, "physics.static");
    w.add_tag(result.trigger_platform, tag_id{"physics.trigger"});
    w.get_component<collision_filter_3d_component>(result.trigger_platform).layer=collision_layer_3d::trigger;
    result.sphere = create_body(w, resources, "physics.sphere", {4,5,-1}, {0.65f,0.65f,0.65f},
        {1,0.4f,0.10f,1}, "physics.dynamic", true);

    result.filter_platform = create_body(w,resources,"physics.filter_platform",{-6,2,2},{1.1f,0.12f,1.1f},
        {0.1f,0.7f,0.75f,1},"physics.static");
    w.get_component<collision_filter_3d_component>(result.filter_platform).layer=collision_layer_3d::enemy;
    result.filtered_cube = create_body(w,resources,"physics.filtered_cube",{-6,5,2},{0.5f,0.5f,0.5f},
        {0.9f,0.9f,0.95f,1},"physics.dynamic");

    result.directional_light = w.create_entity({.name=string_id{"entity.sun"}});
    w.add_component<directional_light_3d_component>(result.directional_light,
        directional_light_3d_component{.direction={0.35f,-0.9f,0.3f}, .intensity=3.0f});
    w.add_tag(result.directional_light, tag_id{"light.directional"});
    w.add_tag(result.directional_light, tag_id{"light.active"});
    reset_sample_3d_world(w, result);
    return result;
}

void reset_sample_3d_world(world& w, const sample_3d_world_handles& h) {
    w.get_component<transform_3d_component>(h.camera).position={0,6,-15};
    w.get_component<transform_3d_component>(h.camera).rotation={0.24f,0,0};
    w.get_component<velocity_3d_component>(h.camera).linear={};
    const auto reset_body = [&](entity_id e, vec3 position) {
        w.get_component<transform_3d_component>(e).position=position;
        w.get_component<transform_3d_component>(e).rotation={};
        w.get_component<velocity_3d_component>(e).linear={};
        w.remove_tag(e, tag_id{"state.frozen"});
        w.remove_tag(e, tag_id{"physics.no_gravity"});
        w.remove_tag(e, tag_id{"physics.disabled"});
    };
    reset_body(h.cube, {-4,4,0});
    reset_body(h.frozen_cube, {-1.5f,3,0});
    reset_body(h.floating_cube, {1,3,2.5f});
    reset_body(h.kinematic_cube, {-3,0.5f,3});
    reset_body(h.sphere, {4,5,-1});
    reset_body(h.filtered_cube, {-6,5,2});
    w.get_component<collision_filter_3d_component>(h.filtered_cube).mask=collision_layer_3d::world;
    w.add_tag(h.frozen_cube, tag_id{"state.frozen"});
    w.add_tag(h.floating_cube, tag_id{"physics.no_gravity"});
    w.get_component<velocity_3d_component>(h.kinematic_cube).linear={1.5f,0,0};
    // Repeated resets must not accumulate reference-counted tags.
    if (!w.has_tag(h.trigger_platform, tag_id{"physics.trigger"}))
        w.add_tag(h.trigger_platform, tag_id{"physics.trigger"});
}
}
