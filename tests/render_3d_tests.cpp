#include <rain/asset/gltf_model_3d.hpp>
#include <rain/render/frustum_culling_3d.hpp>
#include <rain/render/render_system_3d.hpp>
#include <rain/runtime/transform_hierarchy_system_3d.hpp>
#include <rain/runtime/world_transform_3d_component.hpp>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace rain;
#define CHECK(condition) do { if (!(condition)) throw std::runtime_error(     std::string{__func__} + ":" + std::to_string(__LINE__) + " " #condition); } while (false)

void test_pbr_shader();

namespace {
struct recording_backend final : render_backend {
    struct buffer { std::vector<unsigned char> bytes; };
    handle_pool<render_buffer_handle_tag, buffer> buffers;
    handle_pool<pipeline_state_handle_tag, pipeline_state_desc> pipelines;
    handle_pool<texture_2d_handle_tag, texture_format> textures;
    struct draw_record {
        pipeline_state_desc pipeline;
        std::vector<unsigned char> object, material;
        texture_2d_handle albedo, mr;
    };
    std::vector<draw_record> draws;
    pipeline_state_handle current_pipeline;
    render_buffer_handle object_buffer, material_buffer;
    texture_2d_handle bound[2]{};
    u32 viewport_width = 1600, viewport_height = 900;
    bool fail_index_buffer = false;

    void begin_frame() override {}
    void clear(const render_clear_color&) override {}
    void clear_depth(f32) override {}
    void draw_debug_triangle() override {}
    void end_frame() override {}
    void resize(u32 w, u32 h) override { viewport_width=w; viewport_height=h; }
    u32 width() const override { return viewport_width; }
    u32 height() const override { return viewport_height; }
    shader_program_handle create_shader_program(const shader_program_desc&) override { return {0,0}; }
    render_buffer_handle make_buffer(const render_buffer_desc& desc) {
        buffer value{std::vector<unsigned char>(desc.size_bytes)};
        if (desc.initial_data) std::memcpy(value.bytes.data(), desc.initial_data, desc.size_bytes);
        return buffers.create(std::move(value));
    }
    render_buffer_handle create_vertex_buffer(const render_buffer_desc& desc) override { return make_buffer(desc); }
    render_buffer_handle create_index_buffer(const render_buffer_desc& desc) override {
        return fail_index_buffer ? render_buffer_handle{} : make_buffer(desc);
    }
    render_buffer_handle create_constant_buffer(const render_buffer_desc& desc) override { return make_buffer(desc); }
    pipeline_state_handle create_pipeline_state(const pipeline_state_desc& desc) override { return pipelines.create(desc); }
    texture_2d_handle create_texture_2d(const texture_2d_desc& desc) override { return textures.create(desc.format); }
    bool destroy_shader_program(shader_program_handle) override { return true; }
    bool destroy_render_buffer(render_buffer_handle h) override { return buffers.destroy(h); }
    bool destroy_pipeline_state(pipeline_state_handle h) override { return pipelines.destroy(h); }
    bool destroy_texture_2d(texture_2d_handle h) override { return textures.destroy(h); }
    bool is_valid(shader_program_handle h) const override { return h.is_valid(); }
    bool is_valid(render_buffer_handle h) const override { return buffers.is_valid(h); }
    bool is_valid(pipeline_state_handle h) const override { return pipelines.is_valid(h); }
    bool is_valid(texture_2d_handle h) const override { return textures.is_valid(h); }
    void flush_resource_destruction() override {}
    void set_pipeline_state(pipeline_state_handle h) override { current_pipeline=h; }
    void set_vertex_buffer(render_buffer_handle) override {}
    void set_vertex_buffer(render_buffer_handle, u32) override {}
    void set_index_buffer(render_buffer_handle, render_index_format) override {}
    void set_vertex_constant_buffer(render_buffer_handle h, u32 slot) override { if (slot==0) object_buffer=h; }
    void set_pixel_constant_buffer(render_buffer_handle h, u32 slot) override { if (slot==2) material_buffer=h; }
    void update_buffer(render_buffer_handle h, const void* data, usize size) override {
        auto& bytes=buffers.get(h).bytes;
        CHECK(bytes.size()==size);
        std::memcpy(bytes.data(),data,size);
    }
    void set_texture_2d(texture_2d_handle h,u32 slot) override { if (slot<2) bound[slot]=h; }
    void draw(u32,u32) override {}
    void draw_indexed(u32,u32,i32) override {
        draws.push_back({pipelines.get(current_pipeline),buffers.get(object_buffer).bytes,
                         buffers.get(material_buffer).bytes,bound[0],bound[1]});
    }
};

bool close(f32 a, f32 b, f32 tolerance=1.0e-4f) { return std::abs(a-b)<tolerance; }

entity_id camera_entity(world& w) {
    const auto e=w.create_entity({});
    w.add_component<world_transform_3d_component>(e,world_transform_3d_component{});
    w.add_component<camera_3d_component>(e,camera_3d_component{.near_plane=1.0f,.far_plane=50.0f});
    w.add_tag(e,tag_id{"camera.3d"});
    w.add_tag(e,tag_id{"camera.active"});
    return e;
}

entity_id object_entity(world& w,mesh_3d_handle mesh,material_3d_handle material,vec3 position) {
    const auto e=w.create_entity({});
    w.add_component<world_transform_3d_component>(e,world_transform_3d_component{
        .matrix=make_translation(position),.position=position});
    w.add_component<mesh_3d_component>(e,mesh_3d_component{.mesh=mesh,.material=material});
    w.add_tag(e,tag_id{"object.renderable"});
    w.add_tag(e,tag_id{"render.3d"});
    w.add_tag(e,tag_id{"render.frustum_cull"});
    return e;
}

void test_frustum_planes() {
    for (const f32 aspect : {0.5f,1.0f,2.5f})
    for (const f32 fov : {0.6f,1.4f}) {
        const auto projection=make_perspective_fov_lh(fov,aspect,1,40);
        const auto frustum=extract_camera_frustum(projection);
        CHECK(sphere_inside_camera_frustum({{0,0,10},0},frustum));
        CHECK(!sphere_inside_camera_frustum({{0,0,-10},1},frustum));
        CHECK(sphere_inside_camera_frustum({{0,0,0},1},frustum));
        CHECK(sphere_inside_camera_frustum({{0,0,41},1},frustum));
        CHECK(!sphere_inside_camera_frustum({{0,0,42},1},frustum));
        // Every side plane, both orientations: tangent spheres remain visible.
        for (usize p=0;p<4;++p) {
            const auto& plane=frustum.planes[p];
            const vec3 inside{0,0,10};
            const f32 distance=dot(plane.normal,inside)+plane.distance;
            const vec3 tangent=inside-plane.normal*(distance+0.25f);
            CHECK(sphere_inside_camera_frustum({tangent,0.25f},frustum));
            CHECK(!sphere_inside_camera_frustum({tangent-plane.normal*0.01f,0.25f},frustum));
        }
        // Independent clip-space point oracle exercises row/column and z conventions.
        const mat4 view=make_look_at_lh({3,2,-5},{0,1,5},{0,1,0});
        const mat4 vp=view*projection;
        const auto rotated=extract_camera_frustum(vp);
        for (int x=-7;x<=7;++x) for (int y=-5;y<=5;++y) for (int z=-6;z<45;z+=3) {
            const vec3 point{static_cast<f32>(x),static_cast<f32>(y),static_cast<f32>(z)};
            f32 clip[4]{};
            for (usize col=0;col<4;++col)
                clip[col]=point.x*vp.values[0][col]+point.y*vp.values[1][col]+point.z*vp.values[2][col]+vp.values[3][col];
            const bool inside=clip[0]>=-clip[3] && clip[0]<=clip[3] &&
                clip[1]>=-clip[3] && clip[1]<=clip[3] && clip[2]>=0 && clip[2]<=clip[3];
            if (inside) CHECK(sphere_inside_camera_frustum({point,0},rotated));
            if (!sphere_inside_camera_frustum({point,0},rotated)) CHECK(!inside);
        }
    }
    const auto invalid=std::numeric_limits<f32>::quiet_NaN();
    CHECK(sphere_inside_camera_frustum({{0,0,0},invalid},camera_frustum_3d{}));
}

void test_bounds() {
    recording_backend backend;
    mesh_3d_registry meshes(backend);
    const mesh_vertex_3d vertices[]={{{-4,-2,-1},{},{}} ,{{6,8,3},{},{}},{{0,0,0},{},{}}};
    const u32 indices[]={0,1,2};
    auto handle=meshes.create({.name="test.mesh",.vertices=vertices,.indices=indices});
    CHECK(handle.is_valid());
    const auto bounds=meshes.try_get(handle)->local_bounds;
    CHECK(close(bounds.center.x,1) && close(bounds.center.y,3) && close(bounds.center.z,1));
    for (const auto& v:vertices) CHECK(length(v.position-bounds.center)<=bounds.radius+1.0e-5f);

    mat4 shear=mat4::identity(); shear.values[0][1]=1.7f;
    const mat4 transforms[]={make_scale({-2,3,0.5f}),
        make_rotation_y(0.8f)*make_scale({4,0.2f,2})*make_rotation_x(0.4f),
        shear*make_translation({2,3,4}),make_scale({0,0,0})};
    for (const auto& transform:transforms) {
        const bounding_sphere_3d local{{1,2,3},2};
        const auto transformed=transform_bounding_sphere(local,transform);
        for (int i=0;i<1000;++i) {
            const f32 angle=static_cast<f32>(i)*0.618f;
            const vec3 direction=normalize(vec3{std::cos(angle),std::sin(angle),std::sin(angle*1.73f)});
            const vec3 p=transform_point(local.center+direction*local.radius,transform);
            CHECK(length(p-transformed.center)<=transformed.radius+1.0e-4f);
        }
    }
    CHECK(meshes.destroy(handle));
    const u32 invalid_indices[]={0,1,8};
    CHECK(!meshes.create({.name="test.mesh",.vertices=vertices,.indices=invalid_indices}).is_valid());
    backend.fail_index_buffer=true;
    const auto buffer_count=backend.buffers.size();
    CHECK(!meshes.create({.name="test.mesh",.vertices=vertices,.indices=indices}).is_valid());
    CHECK(backend.buffers.size()==buffer_count);
}

void test_render_prepare_and_submit() {
    recording_backend backend;
    mesh_3d_registry meshes(backend);
    material_3d_registry materials;
    world w;
    auto camera=camera_entity(w);
    auto cube=meshes.create_cube();
    material_3d_desc desc{};
    desc.metallic_factor=0.7f; desc.roughness_factor=0.3f; desc.alpha_cutoff=0.4f;
    desc.albedo_texture=backend.create_texture_2d({});
    desc.metallic_roughness_texture=backend.create_texture_2d({});
    const auto material=materials.create(desc);
    auto visible=object_entity(w,cube,material,{0,0,5});
    auto outside=object_entity(w,cube,material,{100,0,5});
    object_entity(w,{},material,{0,0,5});
    const auto parent=w.create_entity({});
    auto hidden=object_entity(w,cube,material,{0,0,5});
    w.set_parent(hidden,parent);
    w.add_tag(parent,tag_id{"render.hidden"});
    {
        render_system_3d renderer(backend,meshes,materials);
        renderer.prepare(w);
        CHECK(renderer.stats().candidate_count==2);
        CHECK(renderer.stats().visible_count==1 && renderer.stats().culled_count==1);
        renderer.submit();
        CHECK(backend.draws.size()==1);
        CHECK(backend.draws[0].material.size()==48 && backend.draws[0].object.size()==192);
        float constants[12]{};
        std::memcpy(constants,backend.draws[0].material.data(),sizeof(constants));
        CHECK(close(constants[4],0.7f) && close(constants[5],0.3f) && close(constants[6],0.4f));
        CHECK(constants[8]==1 && constants[9]==1 && constants[10]==0);
        CHECK(backend.draws[0].mr==desc.metallic_roughness_texture);
        CHECK(backend.draws[0].pipeline.depth_write_enabled);
        CHECK(backend.draws[0].pipeline.srgb_write_enabled);

        w.add_tag(outside,tag_id{"render.always_visible"});
        renderer.prepare(w);
        CHECK(renderer.stats().visible_count==2 && renderer.stats().culled_count==0);
        w.remove_tag(outside,tag_id{"render.always_visible"});
        // Opt-in behavior is retained; inherited culling is also supported.
        w.remove_tag(outside,tag_id{"render.frustum_cull"});
        renderer.prepare(w);
        CHECK(renderer.stats().visible_count==2);
        w.set_parent(outside,parent);
        w.remove_tag(parent,tag_id{"render.hidden"});
        w.add_tag(parent,tag_id{"render.frustum_cull"});
        renderer.prepare(w);
        CHECK(renderer.stats().candidate_count==3 && renderer.stats().culled_count==1);
        w.add_tag(parent,tag_id{"render.always_visible"});
        renderer.prepare(w);
        CHECK(renderer.stats().visible_count==3);
        w.add_tag(parent,tag_id{"render.hidden"});

        // Opaque first; transparent sorted by view depth, not radial distance.
        desc.blend_mode=render_blend_mode::alpha;
        desc.double_sided=true;
        desc.metallic_roughness_texture={};
        const auto transparent=materials.create(desc);
        auto near=object_entity(w,cube,transparent,{7,0,8});
        object_entity(w,cube,transparent,{0,0,10});
        auto& near_transform=w.get_component<world_transform_3d_component>(near);
        near_transform.matrix=make_scale({-1,2,1})*make_translation({7,0,8});
        renderer.prepare(w); backend.draws.clear(); renderer.submit();
        CHECK(backend.draws.size()==3);
        CHECK(backend.draws[0].pipeline.blend_mode==render_blend_mode::opaque);
        mat4 far_matrix{},near_matrix{};
        std::memcpy(&far_matrix,backend.draws[1].object.data(),sizeof(mat4));
        std::memcpy(&near_matrix,backend.draws[2].object.data(),sizeof(mat4));
        CHECK(close(far_matrix.values[3][2],10) && close(near_matrix.values[3][2],8));
        CHECK(!backend.draws[1].pipeline.depth_write_enabled && backend.draws[1].pipeline.depth_test_enabled);
        CHECK(backend.draws[2].pipeline.front_counter_clockwise);
        CHECK(backend.draws[2].pipeline.cull_mode==render_cull_mode::none);
        CHECK(!backend.draws[1].mr.is_valid()); // No stale MR slot from the opaque draw.
        w.get_component<mesh_3d_component>(visible).material={};
        renderer.prepare(w); backend.draws.clear(); renderer.submit();
        CHECK(backend.draws.size()==3); // Stale material falls back to default.
        renderer.clear(); CHECK(renderer.stats().candidate_count==0 && renderer.command_count()==0);
        backend.resize(0,900); renderer.prepare(w); CHECK(renderer.command_count()==0);
        backend.resize(1600,900);
        w.get_component<camera_3d_component>(camera).far_plane=0;
        renderer.prepare(w); CHECK(renderer.command_count()==0);
        w.remove_tag(camera,tag_id{"camera.active"});
        renderer.prepare(w); CHECK(renderer.stats().visible_count==0);
    }
    CHECK(backend.pipelines.empty());
    CHECK(meshes.destroy(cube));
}

void test_transform_hierarchy_culling() {
    recording_backend backend; mesh_3d_registry meshes(backend); material_3d_registry materials;
    world w; const auto cube=meshes.create_cube();
    const auto add_local=[&](vec3 position,vec3 rotation,vec3 scale) {
        const auto e=w.create_entity({});
        w.add_component<transform_3d_component>(e,transform_3d_component{
            .position=position,.rotation=rotation,.scale=scale});
        w.add_tag(e,tag_id{"transform.3d"}); return e;
    };
    const auto camera=add_local({},{},{1,1,1});
    w.add_component<camera_3d_component>(camera,camera_3d_component{});
    w.add_tag(camera,tag_id{"camera.3d"}); w.add_tag(camera,tag_id{"camera.active"});
    const auto parent=add_local({0,0,8},{},{2,0.5f,1});
    const auto child=add_local({},{0,0.6f,0.3f},{1,1,1});
    w.set_parent(child,parent); w.add_tag(child,tag_id{"transform.inherit_parent"});
    w.add_component<mesh_3d_component>(child,mesh_3d_component{.mesh=cube,.material=materials.default_material()});
    w.add_tag(child,tag_id{"object.renderable"}); w.add_tag(child,tag_id{"render.3d"});
    w.add_tag(parent,tag_id{"render.frustum_cull"});
    entity_query_desc query{.required_components={get_type_id<transform_3d_component>()},.required_tags={}};
    query.required_tags.require_all(tag_id{"transform.3d"});
    system_context update{.target_world=&w,.entity_query=&query};
    render_system_3d renderer(backend,meshes,materials);
    transform_hierarchy_system_3d(update,nullptr); renderer.prepare(w);
    CHECK(renderer.stats().visible_count==1);
    w.get_component<transform_3d_component>(parent).position.x=200;
    transform_hierarchy_system_3d(update,nullptr); renderer.prepare(w);
    CHECK(renderer.stats().culled_count==1 && renderer.command_count()==0);
    CHECK(meshes.destroy(cube));
}

void test_gltf_materials() {
    // Self-contained fixture with external image and buffer. It never depends on user assets.
    const auto folder=std::filesystem::path{"gltf_pbr_test_fixture"};
    std::filesystem::create_directories(folder);
    const float positions[]={-1,-1,0, 1,-1,0, 0,1,0};
    std::ofstream bin(folder/"mesh.bin",std::ios::binary);
    bin.write(reinterpret_cast<const char*>(positions),sizeof(positions)); bin.close();
    // A 1x1 uncompressed TGA is supported by the existing image loader.
    unsigned char tga[21]{}; tga[2]=2; tga[12]=1; tga[14]=1; tga[16]=24;
    tga[18]=64; tga[19]=128; tga[20]=192;
    std::ofstream image(folder/"map image.tga",std::ios::binary);
    image.write(reinterpret_cast<const char*>(tga),sizeof(tga)); image.close();
    std::ofstream json(folder/"model.gltf");
    json << R"({
      "asset":{"version":"2.0"},"buffers":[{"uri":"mesh.bin","byteLength":36}],
      "bufferViews":[{"buffer":0,"byteLength":36}],
      "accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[-1,-1,0],"max":[1,1,0]}],
      "images":[{"uri":"map%20image.tga"}],"textures":[{"source":0}],
      "materials":[{"pbrMetallicRoughness":{"baseColorFactor":[0.2,0.3,0.4,0.6],
        "metallicFactor":0.7,"roughnessFactor":0.25,"baseColorTexture":{"index":0},
        "metallicRoughnessTexture":{"index":0}},"alphaMode":"MASK","alphaCutoff":0.33,"doubleSided":true},
        {"alphaMode":"BLEND"},{}],
      "meshes":[{"primitives":[{"attributes":{"POSITION":0},"material":0},
        {"attributes":{"POSITION":0},"material":1},{"attributes":{"POSITION":0},"material":2},
        {"attributes":{"POSITION":0}},{"attributes":{"POSITION":0}}]}],
      "nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0
    })";
    json.close();
    recording_backend backend; mesh_3d_registry meshes(backend); material_3d_registry materials;
    texture_asset_registry textures(backend); world w;
    auto instance=instantiate_gltf_model_3d((folder/"model.gltf").string().c_str(),w,meshes,materials,textures);
    CHECK(instance.owned_meshes.size()==5 && instance.owned_materials.size()==4);
    const auto& m=*materials.try_get(instance.owned_materials[0]);
    CHECK(close(m.metallic_factor,0.7f) && close(m.roughness_factor,0.25f));
    CHECK(close(m.alpha_cutoff,0.33f) && m.double_sided && close(m.base_color.w,0.6f));
    CHECK(!m.albedo_manual_srgb_decode);
    CHECK(m.albedo_texture!=m.metallic_roughness_texture);
    CHECK(backend.textures.get(m.albedo_texture)==texture_format::rgba8_unorm_srgb);
    CHECK(backend.textures.get(m.metallic_roughness_texture)==texture_format::rgba8_unorm);
    CHECK(textures.records().size()==2);
    const auto albedo=m.albedo_texture, mr=m.metallic_roughness_texture;
    const auto image_path=(folder/"map image.tga").generic_string();
    CHECK(textures.load_texture_2d(image_path.c_str(),texture_format::rgba8_unorm_srgb)==albedo);
    CHECK(textures.load_texture_2d(image_path.c_str())==mr);
    CHECK(materials.try_get(instance.owned_materials[1])->blend_mode==render_blend_mode::alpha);
    for (usize i=1;i<4;++i) CHECK(materials.try_get(instance.owned_materials[i])->metallic_factor==1.0f);
    CHECK(textures.unload_texture_2d(image_path.c_str(),texture_format::rgba8_unorm_srgb));
    CHECK(backend.is_valid(mr) && !backend.is_valid(albedo));
    destroy_gltf_model_3d(instance,w,meshes,materials);
    CHECK(instance.entities.empty());
    textures.clear_cache();
    CHECK(backend.textures.empty() && backend.buffers.empty());
}
}

int main() {
    try {
        test_frustum_planes(); std::cout<<"PASS frustum planes, tangency, aspect/FOV and clip oracle\n";
        test_bounds(); std::cout<<"PASS mesh bounds, shear, negative/zero scale and failed uploads\n";
        test_render_prepare_and_submit(); std::cout<<"PASS prepare, stats, hierarchy tags, GPU constants and draw ordering\n";
        test_transform_hierarchy_culling(); std::cout<<"PASS transform hierarchy to culling integration\n";
        test_gltf_materials(); std::cout<<"PASS glTF factors, maps, alpha, defaults and sRGB cache\n";
        test_pbr_shader(); std::cout<<"PASS compiled HLSL and D3D11 WARP pixel regressions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr<<error.what()<<'\n'; return 1;
    }
}
