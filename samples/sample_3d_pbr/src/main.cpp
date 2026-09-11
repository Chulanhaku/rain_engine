#include <rain/app/application.hpp>
#include <rain/core/log.hpp>
#include <rain/render/render_system_3d.hpp>
#include <rain/runtime/transform_hierarchy_system_3d.hpp>

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {
using namespace rain;

mesh_3d_handle create_sphere(mesh_3d_registry& meshes) {
    constexpr u32 rings=48,segments=96;
    constexpr f32 pi=3.14159265359f;
    std::vector<mesh_vertex_3d> vertices;
    std::vector<u32> indices;
    for (u32 ring=0;ring<=rings;++ring) {
        const f32 v=static_cast<f32>(ring)/rings;
        for (u32 segment=0;segment<=segments;++segment) {
            const f32 u=static_cast<f32>(segment)/segments;
            const vec3 normal{std::sin(v*pi)*std::cos(u*2*pi),std::cos(v*pi),
                              std::sin(v*pi)*std::sin(u*2*pi)};
            vertices.push_back({normal,normal,{u,v}});
        }
    }
    for (u32 ring=0;ring<rings;++ring) for (u32 segment=0;segment<segments;++segment) {
        const u32 a=ring*(segments+1)+segment,b=a+segments+1;
        if (ring!=0) indices.insert(indices.end(),{a,a+1,b});
        if (ring+1!=rings) indices.insert(indices.end(),{a+1,b+1,b});
    }
    return meshes.create({.name="pbr.sphere",.vertices=vertices,.indices=indices});
}

class pbr_layer final : public layer {
    std::unique_ptr<render_system_3d> renderer_;
    entity_id camera_;
    entity_query_desc transform_query_;
    mesh_3d_handle sphere_;
    std::vector<entity_id> entities_,cull_objects_;
    std::vector<material_3d_handle> materials_;
    texture_2d_handle mr_texture_;
    u64 frame_limit_=0;
    bool previous_c_=false,culling_enabled_=true;
    render_system_3d_stats last_stats_{};

    entity_id add_transform(world& w,vec3 position,vec3 scale={1,1,1}) {
        const auto e=w.create_entity({});
        entities_.push_back(e);
        w.add_component<transform_3d_component>(e,transform_3d_component{.position=position,.scale=scale});
        w.add_tag(e,tag_id{"transform.3d"});
        return e;
    }
    void add_sphere(application_context& context,vec3 position,material_3d_handle material,
                    vec3 scale={0.75f,0.75f,0.75f}) {
        const auto e=add_transform(*context.target_world,position,scale);
        context.target_world->add_component<mesh_3d_component>(e,mesh_3d_component{.mesh=sphere_,.material=material});
        context.target_world->add_tag(e,tag_id{"object.renderable"});
        context.target_world->add_tag(e,tag_id{"render.3d"});
        context.target_world->add_tag(e,tag_id{"render.frustum_cull"});
        cull_objects_.push_back(e);
    }
public:
    explicit pbr_layer(u64 frame_limit):frame_limit_(frame_limit) {
        transform_query_.required_components = {get_type_id<transform_3d_component>()};
        transform_query_.required_tags.require_all(tag_id{"transform.3d"});
        transform_query_.require_alive = true;
        transform_query_.require_active = true;
    }
    void on_attach(application_context& context) override {
        renderer_=std::make_unique<render_system_3d>(*context.renderer,*context.meshes_3d,*context.materials_3d);
        auto& w=*context.target_world;
        sphere_=create_sphere(*context.meshes_3d);
        camera_=add_transform(w,{0,0,-14});
        w.add_component<camera_3d_component>(camera_,camera_3d_component{.far_plane=100});
        w.add_tag(camera_,tag_id{"camera.3d"}); w.add_tag(camera_,tag_id{"camera.active"});
        const auto sun=w.create_entity({}); entities_.push_back(sun);
        w.add_component<directional_light_3d_component>(sun,directional_light_3d_component{
            .direction={0.4f,-0.7f,1},.color={1,0.96f,0.9f},.intensity=4});
        w.add_tag(sun,tag_id{"light.directional"}); w.add_tag(sun,tag_id{"light.active"});

        for (u32 row=0;row<5;++row) for (u32 column=0;column<6;++column) {
            material_3d_desc desc{};
            desc.name="pbr.grid."+std::to_string(row)+"."+std::to_string(column);
            desc.base_color={0.8f,0.25f,0.08f,1};
            desc.metallic_factor=static_cast<f32>(row)/4;
            desc.roughness_factor=0.08f+0.92f*static_cast<f32>(column)/5;
            auto material=context.materials_3d->create(desc); materials_.push_back(material);
            add_sphere(context,{(static_cast<f32>(column)-2.5f)*2.2f,
                               (2-static_cast<f32>(row))*2.2f,0},material);
        }
        // An untextured model and hundreds of offscreen objects share one mesh.
        for (u32 i=0;i<240;++i)
            add_sphere(context,{(i%2 ? 1.0f : -1.0f)*(60+static_cast<f32>(i%20)),
                static_cast<f32>(i%7)-3,static_cast<f32>(i/20)*3},materials_.front());

        const u8 pixels[]={255,20,255,255,255,80,255,255,255,160,255,255,255,255,255,255};
        mr_texture_=context.renderer->create_texture_2d({.name="pbr.mr.stripes",.width=4,.height=1,
            .pixels=pixels,.size_bytes=sizeof(pixels)});
        material_3d_desc textured{};
        textured.name="pbr.mr.map"; textured.base_color={0.9f,0.7f,0.3f,1};
        textured.metallic_factor=1; textured.metallic_roughness_texture=mr_texture_;
        auto material=context.materials_3d->create(textured); materials_.push_back(material);
        add_sphere(context,{0,-6.3f,1},material,{1,1,1});
        log_info("PBR grid: left->right roughness 0.08..1, top->bottom metallic 0..1.");
        log_info("WASD move, Q/E down/up, arrows look, C toggle culling, R reset, Esc quit.");
    }
    void on_update(application_context& context) override {
        auto& window=*context.main_window;
        if (window.is_key_down(key_code::escape) ||
            (frame_limit_!=0 && context.frame_index>=frame_limit_)) { window.request_close(); return; }
        auto& camera=context.target_world->get_component<transform_3d_component>(camera_);
        const f32 dt=context.delta_seconds;
        const auto axis=[&](key_code positive,key_code negative) {
            return static_cast<f32>(window.is_key_down(positive))-static_cast<f32>(window.is_key_down(negative));
        };
        camera.rotation.y+=axis(key_code::right,key_code::left)*dt;
        camera.rotation.x+=axis(key_code::down,key_code::up)*dt;
        camera.rotation.x=std::clamp(camera.rotation.x,-1.4f,1.4f);
        const mat4 rotation=make_rotation_y(camera.rotation.y);
        const vec3 movement=transform_point({axis(key_code::d,key_code::a),
            axis(key_code::e,key_code::q),axis(key_code::w,key_code::s)},rotation);
        camera.position=camera.position+movement*(dt*7);
        if (window.is_key_down(key_code::r)) { camera.position={0,0,-14}; camera.rotation={}; }
        const bool pressed=window.is_key_down(key_code::c);
        if (pressed && !previous_c_) {
            culling_enabled_=!culling_enabled_;
            for (auto e:cull_objects_) {
                if (culling_enabled_) context.target_world->remove_tag(e,tag_id{"render.always_visible"});
                else context.target_world->add_tag(e,tag_id{"render.always_visible"});
            }
        }
        previous_c_=pressed;
    }
    void on_render(application_context& context) override {
        system_context update{.target_world=context.target_world,.entity_query=&transform_query_};
        transform_hierarchy_system_3d(update,nullptr);
        renderer_->prepare(*context.target_world);
        renderer_->submit();
        const auto stats=renderer_->stats();
        if (stats.visible_count!=last_stats_.visible_count || stats.culled_count!=last_stats_.culled_count) {
            char message[160]{};
            std::snprintf(message,sizeof(message),"3D candidate=%u visible=%u culled=%u culling=%s",
                stats.candidate_count,stats.visible_count,stats.culled_count,culling_enabled_ ? "on" : "off");
            log_info(message); last_stats_=stats;
        }
    }
    void on_detach(application_context& context) override {
        renderer_.reset();
        for (auto e:entities_) if (context.target_world->is_alive(e)) context.target_world->destroy_entity(e);
        for (auto material:materials_) context.materials_3d->destroy(material);
        context.meshes_3d->destroy(sphere_);
        context.renderer->destroy_texture_2d(mr_texture_);
    }
};
}

int main(int argc,char** argv) {
    rain::u64 frame_limit=0;
    if (argc==3 && std::string{argv[1]}=="--frames") frame_limit=std::stoull(argv[2]);
    rain::application app({.title="Rain PBR | roughness -> | metallic down | WASD QE arrows | C culling | R reset",
        .width=1440,.height=900,.clear_color={0.025f,0.03f,0.04f,1}});
    app.push_layer(std::make_unique<pbr_layer>(frame_limit));
    return app.run();
}
