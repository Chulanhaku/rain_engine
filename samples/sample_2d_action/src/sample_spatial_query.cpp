#include "sample_spatial_query.hpp"
#include "sample_physics_3d.hpp"
#include <rain/core/log.hpp>
#include <rain/platform/input_action.hpp>
#include <rain/platform/window.hpp>
#include <rain/render/mesh_3d_component.hpp>
#include <rain/render/picking_3d.hpp>
#include <rain/render/render_backend.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/world_transform_3d_component.hpp>
#include <algorithm>
#include <cmath>
#include <string>

namespace sample_3d {
namespace {
using namespace rain;
void visibility(world& w,entity_id e,bool visible) {
    if (!w.is_alive(e)) return;
    const tag_id hidden{"render.hidden"};
    if (visible && w.has_tag(e,hidden)) w.remove_tag(e,hidden);
    if (!visible && !w.has_tag(e,hidden)) w.add_tag(e,hidden);
}
void marker(world& w,entity_id e,vec3 position,vec3 scale,vec3 rotation,vec4 tint) {
    if (!w.is_alive(e)) return;
    auto& transform=w.get_component<transform_3d_component>(e);
    transform={.position=position,.rotation=rotation,.scale=scale};
    // Markers update after the hierarchy pass. They have no parents/colliders.
    auto& cached=w.get_component<world_transform_3d_component>(e);
    cached.matrix=transform.matrix();cached.position=position;
    w.get_component<mesh_3d_component>(e).tint=tint;
    visibility(w,e,true);
}
void segment(world& w,entity_id e,vec3 from,vec3 to,f32 width,vec4 color) {
    const vec3 delta=to-from;
    const f32 distance=length(delta);
    if (distance<=0.00001f) { visibility(w,e,false);return; }
    const vec3 direction=delta/distance;
    marker(w,e,(from+to)*0.5f,{width,width,distance*0.5f},
        {-std::asin(std::clamp(direction.y,-1.0f,1.0f)),std::atan2(direction.x,direction.z),0},color);
}
}
void spatial_query_demo::initialize(world& w,const sample_3d_world_resources& resources) {
    resources_=resources;
    const auto create=[&](const char* name,bool sphere) {
        const auto e=w.create_entity({.name=string_id{name}});
        w.add_component<transform_3d_component>(e);
        w.add_component<world_transform_3d_component>(e);
        w.add_component<mesh_3d_component>(e,mesh_3d_component{
            .mesh=sphere ? resources.sphere_mesh : resources.cube_mesh,.material=resources.cube_material});
        for (const char* tag:{"transform.3d","object.renderable","render.3d","render.hidden"}) w.add_tag(e,tag_id{tag});
        return e;
    };
    path_marker=create("query.path",false);probe_marker=create("query.probe",true);
    point_marker=create("query.point",true);normal_marker=create("query.normal",false);
}
void spatial_query_demo::restore_tint(world& w) {
    auto* mesh=w.try_get_component<mesh_3d_component>(selected);
    if (!mesh) return;
    if (const auto* feedback=w.try_get_component<physics_feedback_component>(selected))
        mesh->tint=feedback->trigger_contacts>0 ? vec4{0.1f,1,1,1} : feedback->normal_tint;
    else mesh->tint=selected_tint_;
}
void spatial_query_demo::select(world& w,entity_id entity) {
    if (selected==entity) return;
    if (w.is_alive(selected)) w.remove_tag(selected,tag_id{"state.selected"});
    selected=entity;
    if (auto* mesh=w.try_get_component<mesh_3d_component>(selected)) {
        selected_tint_=mesh->tint;
        w.add_tag(selected,tag_id{"state.selected"});
    }
    if (log_selection) {
        if (selected.is_valid() && last_hit)
            log_info("Selected entity "+std::to_string(selected.index)+":"+std::to_string(selected.generation)+
                ", distance="+std::to_string(last_hit->distance)+", normal=("+
                std::to_string(last_hit->normal.x)+","+std::to_string(last_hit->normal.y)+","+
                std::to_string(last_hit->normal.z)+")"+(last_hit->trigger ? " [trigger]" : ""));
        else log_info("Selection cleared");
    }
}
void spatial_query_demo::reset(world& w) {
    restore_tint(w);select(w,{});last_hit.reset();query_count=0;
    for (auto e:{path_marker,probe_marker,point_marker,normal_marker}) visibility(w,e,false);
}
void spatial_query_demo::evaluate(world& w,const ray_3d* ray,bool clicked) {
    restore_tint(w);
    // Destroyed, disabled or hidden selections must not retain highlight/owned Tag.
    if (!w.is_entity_active(selected) || !w.has_component<collider_3d_component>(selected) ||
        !w.has_component<mesh_3d_component>(selected) || w.has_tag_in_hierarchy(selected,tag_id{"physics.disabled"}) ||
        w.has_tag_in_hierarchy(selected,tag_id{"render.hidden"})) select(w,{});
    last_hit.reset();
    for (auto e:{path_marker,probe_marker,point_marker,normal_marker}) visibility(w,e,false);
    if (ray && physics) {
        spatial_query_filter_3d filter;
        filter.triggers=include_triggers ? query_trigger_mode_3d::include : query_trigger_mode_3d::exclude;
        filter.tags.require_all(tag_id{"object.renderable"}).reject(tag_id{"render.hidden"});
        filter.ignored_entities.push_back(camera);
        // Keep the on-screen preview compact; the engine API has no such distance cap.
        auto cast=*ray;cast.max_distance=std::min(cast.max_distance,40.0f);
        const auto& queries=physics->queries();
        constexpr f32 size=0.45f;
        if (mode==spatial_demo_mode::ray) queries.raycast_all(cast,candidates_,filter);
        else if (mode==spatial_demo_mode::sphere)
            queries.sweep_sphere_all({cast.origin,size},cast.direction,cast.max_distance,candidates_,filter);
        else queries.sweep_box_all({cast.origin,{size,size,size}},cast.direction,cast.max_distance,candidates_,filter);
        // Rendering inherits render.hidden from parents. That is a presentation
        // policy, not a physics filter; skip hidden ancestors without blocking hits behind them.
        for (const auto& candidate:candidates_) if (w.is_entity_active(candidate.entity) &&
            !w.has_tag_in_hierarchy(candidate.entity,tag_id{"render.hidden"})) { last_hit=candidate;break; }
        ++query_count;
        if (last_hit && !w.is_alive(last_hit->entity)) last_hit.reset();
        const vec3 end=cast.origin+normalize(cast.direction)*(last_hit ? last_hit->distance : cast.max_distance);
        segment(w,path_marker,cast.origin,end,0.012f,{0.2f,0.8f,1,1});
        if (mode!=spatial_demo_mode::ray && w.is_alive(probe_marker)) {
            w.get_component<mesh_3d_component>(probe_marker).mesh=mode==spatial_demo_mode::sphere ? resources_.sphere_mesh : resources_.cube_mesh;
            marker(w,probe_marker,end,{size,size,size},{},last_hit ? vec4{0.15f,1,0.3f,0.45f} : vec4{0.4f,0.65f,1,0.45f});
        }
        if (last_hit) {
            marker(w,point_marker,last_hit->point,{0.07f,0.07f,0.07f},{},{1,1,1,1});
            segment(w,normal_marker,last_hit->point,last_hit->point+last_hit->normal*0.8f,0.025f,{1,0.3f,0.1f,1});
        }
        if (clicked) select(w,last_hit ? last_hit->entity : entity_id{});
    }
    if (auto* mesh=w.try_get_component<mesh_3d_component>(selected)) mesh->tint={1,0.95f,0.2f,1};
}
void spatial_query_demo::run(system_context& context,void* user_data) {
    if (!context.target_world || !user_data) return;
    auto& demo=*static_cast<spatial_query_demo*>(user_data);
    if (!demo.window || !demo.renderer || !demo.input) return;
    auto& w=*context.target_world;
    for (auto pair:{std::pair{"query.ray",spatial_demo_mode::ray},std::pair{"query.sphere",spatial_demo_mode::sphere},
                   std::pair{"query.box",spatial_demo_mode::box}})
        if (demo.input->is_pressed(string_id{pair.first})) {
            demo.mode=pair.second;log_info(std::string{"Query mode: "}+pair.first);
        }
    if (demo.input->is_pressed(string_id{"query.triggers"})) {
        demo.include_triggers=!demo.include_triggers;
        log_info(demo.include_triggers ? "Query triggers included" : "Query triggers excluded");
    }
    const bool down=demo.window->is_mouse_button_down(mouse_button::left);
    const bool clicked=down && !demo.previous_mouse_down_;
    demo.previous_mouse_down_=down;
    const auto* transform=w.try_get_component<world_transform_3d_component>(demo.camera);
    const auto* camera=w.try_get_component<camera_3d_component>(demo.camera);
    const f32 width=static_cast<f32>(demo.renderer->width()),height=static_cast<f32>(demo.renderer->height());
    vec2 mouse;camera_3d_frame frame;ray_3d ray;
    const bool camera_valid=transform && camera && w.is_entity_active(demo.camera) && height>0 && width>0 &&
        build_camera_frame_3d(*transform,*camera,width/height,frame);
    bool pointer_valid=false;
    bool select_pressed=clicked;
    if (demo.smoke_mode && camera_valid) {
        const auto* target=w.try_get_component<world_transform_3d_component>(demo.smoke_target);
        if (target) {
            const auto ndc=transform_point(target->position,frame.view_projection);
            mouse={(ndc.x+1)*0.5f,(1-ndc.y)*0.5f};pointer_valid=true;
            demo.mode=static_cast<spatial_demo_mode>((context.frame_index/60)%3);
            select_pressed=context.frame_index%60==0;
        }
    } else pointer_valid=demo.window->mouse_position_normalized(mouse);
    if (camera_valid && pointer_valid &&
        screen_point_to_ray_3d(frame,{0,0,width,height},{mouse.x*width,mouse.y*height},ray)) {
        demo.evaluate(w,&ray,select_pressed);
        if (demo.smoke_mode && demo.last_hit && demo.last_hit->entity==demo.smoke_target)
            demo.smoke_modes_hit |= 1u<<static_cast<u32>(demo.mode);
    } else demo.evaluate(w,nullptr,false);
}
}
