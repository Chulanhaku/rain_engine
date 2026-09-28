#include <rain/render/mesh_3d_shader_source.hpp>
#include <rain/render/mesh_3d.hpp>
#include <rain/render/d3d11/d3d11_rasterizer_state.hpp>
#include <rain/core/math/mat4.hpp>
#include <rain/core/math/vec4.hpp>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace rain;
template<class T> struct com {
    T* p=nullptr;
    ~com() { if (p) p->Release(); }
    com()=default;
    com(const com&)=delete;
    com& operator=(const com&)=delete;
    T** out() { if (p) p->Release(); p=nullptr; return &p; }
};
void check(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
void hr(HRESULT result,const char* message) { check(SUCCEEDED(result),message); }

struct warp_renderer {
    com<ID3D11Device> device;
    com<ID3D11DeviceContext> context;
    com<ID3D11VertexShader> vs;
    com<ID3D11PixelShader> ps;
    com<ID3D11InputLayout> layout;
    com<ID3D11Buffer> vertices,object,scene,material;
    com<ID3D11Texture2D> target,staging;
    com<ID3D11RenderTargetView> rtv;
    com<ID3D11RasterizerState> rasterizer;
    com<ID3D11SamplerState> sampler;
    std::array<vec4,4> scene_data{vec4{0,0,1,0},vec4{4,4,4,1},
                                vec4{0.12f,0.12f,0.12f,1},vec4{0,0,-3,1}};
    static constexpr UINT size=32;

    void make_buffer(com<ID3D11Buffer>& output,UINT bind,UINT bytes,const void* data) {
        D3D11_BUFFER_DESC desc{}; desc.ByteWidth=bytes; desc.BindFlags=bind;
        D3D11_SUBRESOURCE_DATA initial{}; initial.pSysMem=data;
        hr(device.p->CreateBuffer(&desc,&initial,output.out()),"CreateBuffer");
    }
    warp_renderer() {
        D3D_FEATURE_LEVEL level{};
        hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,
            D3D11_SDK_VERSION,device.out(),&level,context.out()),"D3D11 WARP unavailable");
        com<ID3DBlob> vs_blob,ps_blob,error;
        const auto compile=[&](const char* entry,const char* profile,com<ID3DBlob>& blob) {
            const auto code=detail::mesh_3d_shader_source;
            const HRESULT result=D3DCompile(code,std::strlen(code),"mesh_3d.hlsl",nullptr,nullptr,
                entry,profile,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS,
                0,blob.out(),error.out());
            if (FAILED(result) && error.p)
                throw std::runtime_error(static_cast<const char*>(error.p->GetBufferPointer()));
            hr(result,"D3DCompile");
        };
        compile("vertex_main","vs_4_0",vs_blob);
        compile("pixel_main","ps_4_0",ps_blob);
        hr(device.p->CreateVertexShader(vs_blob.p->GetBufferPointer(),vs_blob.p->GetBufferSize(),
            nullptr,vs.out()),"CreateVertexShader");
        hr(device.p->CreatePixelShader(ps_blob.p->GetBufferPointer(),ps_blob.p->GetBufferSize(),
            nullptr,ps.out()),"CreatePixelShader");
        const D3D11_INPUT_ELEMENT_DESC attributes[]={
            {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}};
        hr(device.p->CreateInputLayout(attributes,3,vs_blob.p->GetBufferPointer(),
            vs_blob.p->GetBufferSize(),layout.out()),"CreateInputLayout");
        const mesh_vertex_3d triangle[]={{{-1,-1,0},{0,0,-1},{0,0}},
            {{-1,3,0},{0,0,-1},{0,0}},{{3,-1,0},{0,0,-1},{0,0}}};
        make_buffer(vertices,D3D11_BIND_VERTEX_BUFFER,sizeof(triangle),triangle);
        const mat4 matrices[]={mat4::identity(),mat4::identity(),mat4::identity()};
        make_buffer(object,D3D11_BIND_CONSTANT_BUFFER,sizeof(matrices),matrices);
        make_buffer(scene,D3D11_BIND_CONSTANT_BUFFER,sizeof(scene_data),scene_data.data());
        const vec4 zero[3]{};
        make_buffer(material,D3D11_BIND_CONSTANT_BUFFER,sizeof(zero),zero);
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=size; desc.Height=size; desc.MipLevels=1; desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT; desc.SampleDesc.Count=1;
        desc.BindFlags=D3D11_BIND_RENDER_TARGET;
        hr(device.p->CreateTexture2D(&desc,nullptr,target.out()),"Create float render target");
        hr(device.p->CreateRenderTargetView(target.p,nullptr,rtv.out()),"CreateRenderTargetView");
        desc.BindFlags=0; desc.Usage=D3D11_USAGE_STAGING; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        hr(device.p->CreateTexture2D(&desc,nullptr,staging.out()),"Create readback");
        D3D11_RASTERIZER_DESC raster{}; raster.FillMode=D3D11_FILL_SOLID;
        raster.CullMode=D3D11_CULL_NONE; raster.DepthClipEnable=TRUE;
        hr(device.p->CreateRasterizerState(&raster,rasterizer.out()),"CreateRasterizerState");
        D3D11_SAMPLER_DESC sample{}; sample.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;
        sample.AddressU=sample.AddressV=sample.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
        sample.MaxLOD=D3D11_FLOAT32_MAX;
        hr(device.p->CreateSamplerState(&sample,sampler.out()),"CreateSamplerState");
    }
    std::vector<vec4> render(std::array<vec4,3> data,
        std::array<unsigned char,4> mr={255,255,255,255},
        std::array<unsigned char,4> albedo={255,255,255,255},bool srgb=false) {
        com<ID3D11Texture2D> maps[2]; com<ID3D11ShaderResourceView> views[2];
        for (int i=0;i<2;++i) {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width=desc.Height=desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
            desc.Format=i==0 && srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA initial{};
            initial.pSysMem=i==0 ? albedo.data() : mr.data(); initial.SysMemPitch=4;
            hr(device.p->CreateTexture2D(&desc,&initial,maps[i].out()),"Create material map");
            hr(device.p->CreateShaderResourceView(maps[i].p,nullptr,views[i].out()),"Create map view");
        }
        const float clear[4]{};
        context.p->ClearRenderTargetView(rtv.p,clear);
        context.p->OMSetRenderTargets(1,&rtv.p,nullptr);
        const D3D11_VIEWPORT viewport{0,0,static_cast<float>(size),static_cast<float>(size),0,1};
        context.p->RSSetViewports(1,&viewport); context.p->RSSetState(rasterizer.p);
        context.p->IASetInputLayout(layout.p);
        context.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        const UINT stride=sizeof(mesh_vertex_3d),offset=0;
        context.p->IASetVertexBuffers(0,1,&vertices.p,&stride,&offset);
        context.p->VSSetShader(vs.p,nullptr,0); context.p->PSSetShader(ps.p,nullptr,0);
        context.p->UpdateSubresource(material.p,0,nullptr,data.data(),0,0);
        context.p->UpdateSubresource(scene.p,0,nullptr,scene_data.data(),0,0);
        context.p->VSSetConstantBuffers(0,1,&object.p);
        context.p->PSSetConstantBuffers(1,1,&scene.p);
        context.p->PSSetConstantBuffers(2,1,&material.p);
        ID3D11ShaderResourceView* resources[]={views[0].p,views[1].p};
        context.p->PSSetShaderResources(0,2,resources); context.p->PSSetSamplers(0,1,&sampler.p);
        context.p->Draw(3,0);
        context.p->CopyResource(staging.p,target.p);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr(context.p->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"Readback Map");
        std::vector<vec4> result(size*size);
        for (UINT row=0;row<size;++row)
            std::memcpy(result.data()+row*size,static_cast<const char*>(mapped.pData)+row*mapped.RowPitch,
                size*sizeof(vec4));
        context.p->Unmap(staging.p,0);
        for (auto pixel:result)
            check(std::isfinite(pixel.x) && std::isfinite(pixel.y) && std::isfinite(pixel.z) &&
                std::isfinite(pixel.w),"Non-finite PBR pixel");
        return result;
    }
    void check_srgb_blending(std::array<vec4,3> data) {
        // Compare the actual sRGB blend result against encode(linear shader RGB * alpha).
        const auto expected=render(data);
        com<ID3D11Texture2D> color,readback;
        com<ID3D11RenderTargetView> srgb_view;
        com<ID3D11BlendState> blend;
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=desc.Height=size; desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS; desc.BindFlags=D3D11_BIND_RENDER_TARGET;
        hr(device.p->CreateTexture2D(&desc,nullptr,color.out()),"Create sRGB target");
        D3D11_RENDER_TARGET_VIEW_DESC view{};
        view.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; view.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
        hr(device.p->CreateRenderTargetView(color.p,&view,srgb_view.out()),"Create sRGB target view");
        desc.BindFlags=0; desc.Usage=D3D11_USAGE_STAGING; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        hr(device.p->CreateTexture2D(&desc,nullptr,readback.out()),"Create sRGB readback");
        D3D11_BLEND_DESC state{};
        auto& target_blend=state.RenderTarget[0];
        target_blend.BlendEnable=TRUE; target_blend.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
        target_blend.SrcBlend=D3D11_BLEND_SRC_ALPHA; target_blend.DestBlend=D3D11_BLEND_INV_SRC_ALPHA;
        target_blend.BlendOp=target_blend.BlendOpAlpha=D3D11_BLEND_OP_ADD;
        target_blend.SrcBlendAlpha=D3D11_BLEND_ONE; target_blend.DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA;
        hr(device.p->CreateBlendState(&state,blend.out()),"Create alpha blend state");
        const float clear[4]{};
        context.p->ClearRenderTargetView(srgb_view.p,clear);
        context.p->OMSetRenderTargets(1,&srgb_view.p,nullptr);
        context.p->OMSetBlendState(blend.p,nullptr,0xffffffff);
        context.p->Draw(3,0);
        context.p->CopyResource(readback.p,color.p);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr(context.p->Map(readback.p,0,D3D11_MAP_READ,0,&mapped),"Map sRGB result");
        const auto encode=[](float linear) {
            return linear<=0.0031308f ? 12.92f*linear : 1.055f*std::pow(linear,1.0f/2.4f)-0.055f;
        };
        bool correct=true;
        for (UINT row=0;row<size;++row) for (UINT col=0;col<size;++col) {
            const auto* rgba=static_cast<const unsigned char*>(mapped.pData)+row*mapped.RowPitch+col*4;
            const auto p=expected[row*size+col];
            correct=correct && std::abs(rgba[0]/255.0f-encode(p.x*p.w))<0.008f
                && std::abs(rgba[1]/255.0f-encode(p.y*p.w))<0.008f
                && std::abs(rgba[2]/255.0f-encode(p.z*p.w))<0.008f;
        }
        context.p->Unmap(readback.p,0);
        context.p->OMSetBlendState(nullptr,nullptr,0xffffffff);
        context.p->OMSetRenderTargets(1,&rtv.p,nullptr);
        check(correct,"sRGB output or linear-space transparency is incorrect");
    }

};

double difference(const std::vector<vec4>& a,const std::vector<vec4>& b) {
    double sum=0;
    for (usize i=0;i<a.size();++i) sum+=std::abs(a[i].x-b[i].x)+std::abs(a[i].y-b[i].y)+std::abs(a[i].z-b[i].z);
    return sum/a.size();
}
}

void test_pbr_shader() {
    warp_renderer gpu;
    std::array<vec4,3> material{vec4{0.8f,0.25f,0.08f,0.6f},vec4{0,0.4f,-1,0},vec4{}};
    auto dielectric=gpu.render(material);
    material[1].x=1;
    auto metal=gpu.render(material);
    check(difference(metal,dielectric)>0.05,"Metallicity does not change shading");
    material[1].y=0;
    auto smooth=gpu.render(material);
    material[1].y=1;
    auto rough=gpu.render(material);
    check(difference(smooth,rough)>0.01,"Roughness does not change highlight");
    material[2].y=1; // Texture R is unused; G/B modulate material factors.
    auto mapped=gpu.render(material,{5,128,64,255});
    check(difference(mapped,gpu.render(material,{250,128,64,255}))<1.0e-6,"MR red channel affects shading");
    material[2].y=0; material[1].x=64.0f/255; material[1].y=128.0f/255;
    check(difference(mapped,gpu.render(material))<1.0e-5,"MR green/blue channel or factor product is incorrect");

    material[2].x=1; material[1].w=1;
    auto manual=gpu.render(material,{},{128,80,200,153});
    material[1].w=0;
    check(difference(manual,gpu.render(material,{},{128,80,200,153},true))<0.003,
        "Hardware and manual sRGB decode disagree");
    material[2]={}; material[1].z=0.7f;
    const auto clipped=gpu.render(material);
    for (auto pixel:clipped) check(pixel.w==0 && pixel.x==0,"Alpha mask did not discard");
    material[1].z=0.5f;
    for (auto pixel:gpu.render(material)) check(pixel.w==1,"MASK alpha must be opaque");
    material[1].z=-1; material[2].z=1;
    for (auto pixel:gpu.render(material)) check(std::abs(pixel.w-0.6f)<1.0e-6f,"BLEND alpha lost");
    gpu.check_srgb_blending(material);
    gpu.scene_data[0]={}; gpu.scene_data[2]={};
    for (auto pixel:gpu.render(material))
        check(pixel.x==0 && pixel.y==0 && pixel.z==0,"Zero light direction generates illumination");
    std::cout<<"WARP mean RGB differences: metallic="<<difference(metal,dielectric)
             <<", roughness="<<difference(smooth,rough)<<'\n';
}

// Render the real primitive with render_system_3d's actual pipeline settings.
// Read world position so the expected near/far surface is independent of lighting.
void test_cube_rasterization(const mesh_3d_desc& cube,const pipeline_state_desc& normal,
    const pipeline_state_desc& mirrored,const pipeline_state_desc& double_sided) {
    warp_renderer gpu;
    com<ID3D11Buffer> indices;
    gpu.make_buffer(gpu.vertices,D3D11_BIND_VERTEX_BUFFER,static_cast<UINT>(cube.vertices.size_bytes()),cube.vertices.data());
    gpu.make_buffer(indices,D3D11_BIND_INDEX_BUFFER,static_cast<UINT>(cube.indices.size_bytes()),cube.indices.data());
    constexpr char diagnostic_shader[]=R"(
        float4 pixel_main(float4 position : SV_POSITION, float3 world : POSITION1) : SV_Target {
            return float4(world,1);
        }
    )";
    com<ID3DBlob> pixel_blob,error;
    hr(D3DCompile(diagnostic_shader,sizeof(diagnostic_shader)-1,"cube.surface",nullptr,nullptr,
        "pixel_main","ps_4_0",D3DCOMPILE_ENABLE_STRICTNESS,0,pixel_blob.out(),error.out()),"Compile cube surface shader");
    hr(gpu.device.p->CreatePixelShader(pixel_blob.p->GetBufferPointer(),pixel_blob.p->GetBufferSize(),
        nullptr,gpu.ps.out()),"Create cube surface shader");
    com<ID3D11Texture2D> depth_texture;
    com<ID3D11DepthStencilView> depth_view;
    D3D11_TEXTURE2D_DESC depth_desc{};
    depth_desc.Width=depth_desc.Height=warp_renderer::size;
    depth_desc.MipLevels=depth_desc.ArraySize=depth_desc.SampleDesc.Count=1;
    depth_desc.Format=DXGI_FORMAT_D32_FLOAT; depth_desc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
    hr(gpu.device.p->CreateTexture2D(&depth_desc,nullptr,depth_texture.out()),"Create cube depth");
    hr(gpu.device.p->CreateDepthStencilView(depth_texture.p,nullptr,depth_view.out()),"Create cube depth view");
    const D3D11_VIEWPORT viewport{0,0,static_cast<float>(warp_renderer::size),static_cast<float>(warp_renderer::size),0,1};
    gpu.context.p->RSSetViewports(1,&viewport);
    gpu.context.p->IASetInputLayout(gpu.layout.p);
    gpu.context.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    const UINT stride=sizeof(mesh_vertex_3d),offset=0;
    gpu.context.p->IASetVertexBuffers(0,1,&gpu.vertices.p,&stride,&offset);
    gpu.context.p->IASetIndexBuffer(indices.p,DXGI_FORMAT_R32_UINT,0);
    gpu.context.p->VSSetShader(gpu.vs.p,nullptr,0);
    gpu.context.p->PSSetShader(gpu.ps.p,nullptr,0);
    gpu.context.p->VSSetConstantBuffers(0,1,&gpu.object.p);
    gpu.context.p->OMSetRenderTargets(1,&gpu.rtv.p,depth_view.p);
    const vec3 directions[]={{-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1}};
    const char* modes[]={"back culling","mirrored back culling","double sided","front culling"};
    for (usize mode=0;mode<4;++mode) {
        auto pipeline=mode==1 ? mirrored : mode==2 ? double_sided : normal;
        if (mode==3) pipeline.cull_mode=render_cull_mode::front;
        const auto raster_desc=detail::make_d3d11_rasterizer_desc(pipeline);
        hr(gpu.device.p->CreateRasterizerState(&raster_desc,gpu.rasterizer.out()),"Create production cube rasterizer");
        gpu.context.p->RSSetState(gpu.rasterizer.p);
        D3D11_DEPTH_STENCIL_DESC depth_state{};
        depth_state.DepthEnable=pipeline.depth_test_enabled;
        depth_state.DepthWriteMask=pipeline.depth_write_enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        depth_state.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
        com<ID3D11DepthStencilState> depth;
        hr(gpu.device.p->CreateDepthStencilState(&depth_state,depth.out()),"Create cube depth state");
        gpu.context.p->OMSetDepthStencilState(depth.p,0);
        for (usize face=0;face<6;++face) {
            const auto direction=directions[face];
            const auto world=mode==1 ? make_scale({-1,1,1}) : mat4::identity();
            const auto view=make_look_at_lh(direction*4,{},std::abs(direction.y)>0.5f ? vec3{0,0,1} : vec3{0,1,0});
            const mat4 matrices[]={world,world*view*make_perspective_fov_lh(1.04719755f,1,0.1f,20),make_normal_matrix(world)};
            gpu.context.p->UpdateSubresource(gpu.object.p,0,nullptr,matrices,0,0);
            const float clear[4]{};
            gpu.context.p->ClearRenderTargetView(gpu.rtv.p,clear);
            gpu.context.p->ClearDepthStencilView(depth_view.p,D3D11_CLEAR_DEPTH,1,0);
            gpu.context.p->DrawIndexed(static_cast<UINT>(cube.indices.size()),0,0);
            gpu.context.p->CopyResource(gpu.staging.p,gpu.target.p);
            D3D11_MAPPED_SUBRESOURCE mapped{};
            hr(gpu.context.p->Map(gpu.staging.p,0,D3D11_MAP_READ,0,&mapped),"Read cube surface");
            vec4 pixel;
            std::memcpy(&pixel,static_cast<const char*>(mapped.pData)+(warp_renderer::size/2)*mapped.RowPitch+
                (warp_renderer::size/2)*sizeof(vec4),sizeof(pixel));
            gpu.context.p->Unmap(gpu.staging.p,0);
            const float expected=mode==3 ? -1.0f : 1.0f;
            const float surface=dot(vec3{pixel.x,pixel.y,pixel.z},direction);
            if (pixel.w!=1 || std::abs(surface-expected)>0.0001f)
                throw std::runtime_error(std::string{"Cube "}+modes[mode]+" face "+std::to_string(face)+
                    " selected wrong surface: expected "+std::to_string(expected)+", got "+std::to_string(surface));
        }
    }
}
