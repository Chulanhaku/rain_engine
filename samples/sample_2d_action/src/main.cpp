#include "sample_2d_world_builder.hpp"
#include "sample_3d_world_builder.hpp"
#include "sample_physics_3d.hpp"

#include <rain/app/application.hpp>
#include <rain/core/log.hpp>
#include <rain/platform/key_code.hpp>
#include <rain/render/camera_2d.hpp>
#include <rain/render/render_system_2d.hpp>
#include <rain/render/render_system_3d.hpp>
#include <rain/runtime/movement_system_2d.hpp>
#include <rain/runtime/collision_filter_3d_component.hpp>
#include <rain/runtime/transform_2d_component.hpp>
#include <rain/runtime/velocity_2d_component.hpp>
#include <rain/runtime/velocity_3d_component.hpp>

#include <charconv>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace rain;

mesh_3d_handle create_sphere(mesh_3d_registry& meshes) {
    constexpr u32 rings=48, segments=96;
    constexpr f32 pi=3.14159265359f;
    std::vector<mesh_vertex_3d> vertices;
    std::vector<u32> indices;
    for (u32 ring=0; ring<=rings; ++ring) {
        const f32 v=static_cast<f32>(ring)/rings;
        for (u32 segment=0; segment<=segments; ++segment) {
            const f32 u=static_cast<f32>(segment)/segments;
            const vec3 normal{std::sin(v*pi)*std::cos(u*2*pi), std::cos(v*pi),
                              std::sin(v*pi)*std::sin(u*2*pi)};
            vertices.push_back({normal, normal, {u,v}});
        }
    }
    for (u32 ring=0; ring<rings; ++ring) for (u32 segment=0; segment<segments; ++segment) {
        const u32 a=ring*(segments+1)+segment, b=a+segments+1;
        if (ring!=0) indices.insert(indices.end(), {a,a+1,b});
        if (ring+1!=rings) indices.insert(indices.end(), {a+1,b+1,b});
    }
    return meshes.create({.name="physics.sphere", .vertices=vertices, .indices=indices});
}

void sample_bounce_system(system_context& context, void*) {
    if (!context.target_world || !context.entity_query) return;
    auto& w=*context.target_world;
    for (auto e : w.query_entities(*context.entity_query)) {
        auto& transform=w.get_component<transform_2d_component>(e);
        auto& velocity=w.get_component<velocity_2d_component>(e);
        if (transform.position.x>300) { transform.position.x=300; velocity.x=-std::abs(velocity.x); }
        if (transform.position.x< -300) { transform.position.x=-300; velocity.x=std::abs(velocity.x); }
    }
}

class sample_layer final : public layer {
public:
    explicit sample_layer(u64 frame_limit) : frame_limit_(frame_limit) {}

    void on_attach(application_context& context) override {
        bind_input_actions(*context.input);
        auto& w=*context.target_world;
        cube_mesh_=context.meshes_3d->create_cube();
        sphere_mesh_=create_sphere(*context.meshes_3d);
        cube_material_=context.materials_3d->create(material_3d_desc{
            .name="physics.pbr", .metallic_factor=0.15f, .roughness_factor=0.42f});
        renderer_3d_=std::make_unique<render_system_3d>(
            *context.renderer, *context.meshes_3d, *context.materials_3d);
        handles_3d_=sample_3d::build_sample_3d_world(w, {cube_mesh_,cube_material_,sphere_mesh_});
        physics_.log_events=true;
        sample_3d::register_sample_3d_systems(*context.scheduler, physics_, context.input);
        context.scheduler->add_system({.system_name="system.render_prepare_3d", .owner_name="sample_2d_action",
            .phase=system_phase::render_prepare, .priority=10, .entity_query={},
            .function=render_prepare_system_3d, .user_data=renderer_3d_.get()});

        // Keep the original 2D exercise available with Tab, clear of the 3D view by default.
        solid_material_=context.materials->create(material_2d_desc{.name="sample.solid", .texture={}});
        texture_2d_handle image_texture;
        if (std::filesystem::exists("assets/textures/test.jpg"))
            image_texture=context.assets->load_texture_2d("assets/textures/test.jpg");
        image_material_=context.materials->create(material_2d_desc{
            .name="sample.image", .texture=image_texture});
        handles_2d_=sample_2d::build_sample_2d_world(w, {solid_material_,image_material_});
        renderer_2d_=std::make_unique<render_system_2d>(*context.renderer,*context.materials,4096);
        entity_query_desc movement_query;
        movement_query.required_components={get_type_id<transform_2d_component>(),get_type_id<velocity_2d_component>()};
        movement_query.required_tags.require_all(tag_id{"object.movable"})
            .reject(tag_id{"state.frozen"}).reject(tag_id{"state.stunned"}).reject(tag_id{"state.rooted"});
        context.scheduler->add_system({.system_name="system.movement_2d", .owner_name="sample_2d_action",
            .phase=system_phase::movement, .entity_query=movement_query, .function=movement_system_2d});
        context.scheduler->add_system({.system_name="sample.bounce", .owner_name="sample_2d_action",
            .phase=system_phase::movement, .priority=-10, .entity_query=movement_query, .function=sample_bounce_system});
        context.scheduler->add_system({.system_name="system.render_prepare_2d", .owner_name="sample_2d_action",
            .phase=system_phase::render_prepare, .entity_query=render_system_2d::make_entity_query(),
            .function=render_prepare_system_2d, .user_data=renderer_2d_.get()});

        log_info("Physics World: blue=dynamic, yellow=frozen, purple=no gravity, green=kinematic.");
        log_info("Orange sphere turns cyan with state.in_trigger while crossing the red trigger; enter/exit events are logged.");
        log_info("White cube ignores the cyan platform by collision mask, but lands on the ground. L toggles platform collisions.");
        log_info("WASD move; Q/E down/up; arrows look; Shift boost; Esc quit.");
        log_info("Space launch blue; F freeze blue; V release/freeze yellow; G purple gravity; T platform solidity.");
        log_info("P toggle blue physics.disabled; R reset scene + camera; Tab show/hide 2D; H hide/show green 2D rectangle; B freeze 2D movement.");
        for (const auto& info : context.scheduler->debug_infos())
            log_info("registered "+info.system_name+" ["+to_string(info.phase)+"]");

    }

    void on_update(application_context& context) override {
        auto& input=*context.input;
        auto& w=*context.target_world;
        if (input.is_pressed(string_id{"app.quit"})) context.main_window->request_close();
        if (input.is_pressed(string_id{"demo.reset"})) {
            sample_3d::reset_sample_3d_world(w,handles_3d_);
            physics_.reset(w);
        }
        if (input.is_pressed(string_id{"demo.launch"})) {
            w.remove_tag(handles_3d_.cube,tag_id{"state.frozen"});
            w.get_component<velocity_3d_component>(handles_3d_.cube).linear.y=7;
        }
        if (input.is_pressed(string_id{"demo.freeze"})) toggle_tag(w,handles_3d_.cube,"state.frozen");
        if (input.is_pressed(string_id{"demo.release"})) toggle_tag(w,handles_3d_.frozen_cube,"state.frozen");
        if (input.is_pressed(string_id{"demo.gravity"})) toggle_tag(w,handles_3d_.floating_cube,"physics.no_gravity");
        if (input.is_pressed(string_id{"demo.trigger"})) toggle_tag(w,handles_3d_.trigger_platform,"physics.trigger");
        if (input.is_pressed(string_id{"demo.disabled"})) toggle_tag(w,handles_3d_.cube,"physics.disabled");
        if (input.is_pressed(string_id{"demo.filter"})) {
            auto& filter=w.get_component<collision_filter_3d_component>(handles_3d_.filtered_cube);
            filter.mask^=collision_layer_3d::enemy;
            auto& transform=w.get_component<transform_3d_component>(handles_3d_.filtered_cube);
            transform.position={-6,5,2};
            w.get_component<velocity_3d_component>(handles_3d_.filtered_cube).linear={};
            log_info((filter.mask & collision_layer_3d::enemy)!=0 ? "White cube: cyan platform enabled" : "White cube: cyan platform ignored");
        }
        if (input.is_pressed(string_id{"demo.overlay"})) show_2d_=!show_2d_;
        if (input.is_pressed(string_id{"demo.hide_2d"})) toggle_tag(w,handles_2d_.green_rect,"render.hidden");
        if (input.is_pressed(string_id{"demo.freeze_2d"})) toggle_tag(w,handles_2d_.moving_rect,"state.frozen");
        camera_2d_.set_viewport_size(static_cast<f32>(context.renderer->width()),
                                     static_cast<f32>(context.renderer->height()));
    }

    void on_render(application_context& context) override {
        renderer_3d_->submit();
        if (show_2d_) renderer_2d_->submit(camera_2d_);
        if (frame_limit_!=0 && context.frame_index+1>=frame_limit_) context.main_window->request_close();
    }

    void on_detach(application_context& context) override {
        const auto& w=*context.target_world;
        log_info("Physics sample finished: fixed steps="+std::to_string(physics_.step_count)+
            ", cube y="+std::to_string(w.get_component<transform_3d_component>(handles_3d_.cube).position.y)+
            ", sphere y="+std::to_string(w.get_component<transform_3d_component>(handles_3d_.sphere).position.y)+
            ", visible="+std::to_string(renderer_3d_->stats().visible_count));
        const auto& stats=physics_.simulation.stats();
        log_info("Physics World: colliders="+std::to_string(stats.collider_count)+
            ", broadphase pairs="+std::to_string(stats.broadphase_pair_count)+
            ", narrowphase tests="+std::to_string(stats.narrowphase_test_count)+
            ", contacts="+std::to_string(stats.collision_count)+
            ", trigger enter/stay/exit="+std::to_string(physics_.trigger_enters)+"/"+
            std::to_string(physics_.trigger_stays)+"/"+std::to_string(physics_.trigger_exits));
        renderer_2d_.reset();
        renderer_3d_.reset();
        context.materials->destroy(solid_material_);
        context.materials->destroy(image_material_);
        context.materials_3d->destroy(cube_material_);
        context.meshes_3d->destroy(cube_mesh_);
        context.meshes_3d->destroy(sphere_mesh_);
    }

private:
    static void toggle_tag(world& w,entity_id e,const char* name) {
        const tag_id tag{name};
        if (w.has_tag(e,tag)) { w.remove_tag(e,tag); log_info(std::string{name}+" OFF"); }
        else { w.add_tag(e,tag); log_info(std::string{name}+" ON"); }
    }

    static void bind_input_actions(input_action_map& input) {
        input.bind_axis(string_id{"camera.move_x"},key_code::a,-1);
        input.bind_axis(string_id{"camera.move_x"},key_code::d,1);
        input.bind_axis(string_id{"camera.move_y"},key_code::q,-1);
        input.bind_axis(string_id{"camera.move_y"},key_code::e,1);
        input.bind_axis(string_id{"camera.move_z"},key_code::s,-1);
        input.bind_axis(string_id{"camera.move_z"},key_code::w,1);
        input.bind_axis(string_id{"camera.look_x"},key_code::left,-1);
        input.bind_axis(string_id{"camera.look_x"},key_code::right,1);
        input.bind_axis(string_id{"camera.look_y"},key_code::up,-1);
        input.bind_axis(string_id{"camera.look_y"},key_code::down,1);
        input.bind_button(string_id{"camera.boost"},key_code::left_shift);
        input.bind_button(string_id{"app.quit"},key_code::escape);
        input.bind_button(string_id{"demo.launch"},key_code::space);
        input.bind_button(string_id{"demo.freeze"},key_code::f);
        input.bind_button(string_id{"demo.release"},key_code::v);
        input.bind_button(string_id{"demo.gravity"},key_code::g);
        input.bind_button(string_id{"demo.trigger"},key_code::t);
        input.bind_button(string_id{"demo.reset"},key_code::r);
        input.bind_button(string_id{"demo.disabled"},key_code::p);
        input.bind_button(string_id{"demo.filter"},key_code::l);
        input.bind_button(string_id{"demo.overlay"},key_code::tab);
        input.bind_button(string_id{"demo.hide_2d"},key_code::h);
        input.bind_button(string_id{"demo.freeze_2d"},key_code::b);
    }

    sample_3d::physics_demo_systems physics_;
    sample_3d::sample_3d_world_handles handles_3d_;
    sample_2d::sample_2d_world_handles handles_2d_;
    std::unique_ptr<render_system_3d> renderer_3d_;
    std::unique_ptr<render_system_2d> renderer_2d_;
    mesh_3d_handle cube_mesh_,sphere_mesh_;
    material_3d_handle cube_material_;
    material_2d_handle solid_material_,image_material_;
    camera_2d camera_2d_;
    u64 frame_limit_=0;
    bool show_2d_=false;
};
}

int main(int argc,char** argv) {
    if (argc==2 && std::string_view{argv[1]}=="--physics-test")
        return sample_3d::run_physics_sample_tests();
    rain::u64 frame_limit=0;
    if (argc!=1) {
        if (argc!=3 || std::string_view{argv[1]}!="--frames") {
            std::fprintf(stderr,"Usage: sample_2d_action [--physics-test | --frames N]\n");
            return 2;
        }
        const std::string_view number{argv[2]};
        const auto result=std::from_chars(number.data(),number.data()+number.size(),frame_limit);
        if (result.ec!=std::errc{} || result.ptr!=number.data()+number.size() || frame_limit==0) {
            std::fprintf(stderr,"--frames requires a positive integer\n");
            return 2;
        }
    }
    rain::application app({.title="Rain Engine - Physics World | WASD + arrows | Space F V G T P L | R reset | Tab 2D",
        .width=1280, .height=720, .resizable=true,
        .clear_color={.r=0.06f,.g=0.08f,.b=0.13f,.a=1}});
    app.push_layer(std::make_unique<sample_layer>(frame_limit));
    return app.run();
}
