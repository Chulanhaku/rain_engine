#include <rain/render/d3d11/d3d11_frame_targets.hpp>

#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
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

struct pixel { unsigned char r,g,b,a; };
struct position { float x,y; };

class fixture {
public:
    com<ID3D11Device> device;
    com<ID3D11DeviceContext> context;
    com<ID3D11InfoQueue> info;
    com<ID3D11VertexShader> vs;
    com<ID3D11PixelShader> ps;
    com<ID3D11InputLayout> layout;
    com<ID3D11RasterizerState> rasterizer;
    com<ID3D11DepthStencilState> depth;
    com<ID3D11Buffer> vertices,constants;
    UINT vertex_count=0;

    fixture() {
        HRESULT result=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_DEBUG,
            nullptr,0,D3D11_SDK_VERSION,device.out(),nullptr,context.out());
        if (FAILED(result)) result=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,
            nullptr,0,D3D11_SDK_VERSION,device.out(),nullptr,context.out());
        hr(result,"D3D11 WARP create");
        device.p->QueryInterface(__uuidof(ID3D11InfoQueue),reinterpret_cast<void**>(info.out()));

        const char* shader=R"(
            cbuffer Draw : register(b0) { float4 color; float4 parameters; };
            float4 vertex_main(float2 position : POSITION) : SV_POSITION {
                return float4(position,parameters.x,1);
            }
            float4 pixel_main() : SV_Target { return color; }
        )";
        com<ID3DBlob> vertex_blob,pixel_blob,error;
        hr(D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"vertex_main","vs_4_0",
            D3DCOMPILE_ENABLE_STRICTNESS,0,vertex_blob.out(),error.out()),"MSAA vertex shader compile");
        hr(D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"pixel_main","ps_4_0",
            D3DCOMPILE_ENABLE_STRICTNESS,0,pixel_blob.out(),error.out()),"MSAA pixel shader compile");
        hr(device.p->CreateVertexShader(vertex_blob.p->GetBufferPointer(),vertex_blob.p->GetBufferSize(),
            nullptr,vs.out()),"MSAA vertex shader create");
        hr(device.p->CreatePixelShader(pixel_blob.p->GetBufferPointer(),pixel_blob.p->GetBufferSize(),
            nullptr,ps.out()),"MSAA pixel shader create");
        const D3D11_INPUT_ELEMENT_DESC attribute{
            "POSITION",0,DXGI_FORMAT_R32G32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0};
        hr(device.p->CreateInputLayout(&attribute,1,vertex_blob.p->GetBufferPointer(),
            vertex_blob.p->GetBufferSize(),layout.out()),"MSAA input layout");
        D3D11_RASTERIZER_DESC raster{};
        raster.FillMode=D3D11_FILL_SOLID; raster.CullMode=D3D11_CULL_NONE;
        raster.DepthClipEnable=TRUE; raster.MultisampleEnable=TRUE;
        hr(device.p->CreateRasterizerState(&raster,rasterizer.out()),"MSAA raster state");
        D3D11_DEPTH_STENCIL_DESC depth_desc{};
        depth_desc.DepthEnable=TRUE; depth_desc.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;
        depth_desc.DepthFunc=D3D11_COMPARISON_LESS;
        hr(device.p->CreateDepthStencilState(&depth_desc,depth.out()),"MSAA depth state");

        std::vector<position> circle;
        constexpr int segments=192;
        constexpr float pi=3.14159265359f;
        for (int i=0;i<segments;++i) {
            const float a=2*pi*static_cast<float>(i)/segments;
            const float b=2*pi*static_cast<float>(i+1)/segments;
            circle.push_back({0.013f,0.007f});
            circle.push_back({0.013f+0.76f*std::cos(a),0.007f+0.76f*std::sin(a)});
            circle.push_back({0.013f+0.76f*std::cos(b),0.007f+0.76f*std::sin(b)});
        }
        vertex_count=static_cast<UINT>(circle.size());
        D3D11_BUFFER_DESC buffer{};
        buffer.ByteWidth=static_cast<UINT>(circle.size()*sizeof(position));
        buffer.BindFlags=D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA data{}; data.pSysMem=circle.data();
        hr(device.p->CreateBuffer(&buffer,&data,vertices.out()),"MSAA vertex buffer");
        buffer.ByteWidth=32; buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        hr(device.p->CreateBuffer(&buffer,nullptr,constants.out()),"MSAA constants");
    }

    std::vector<pixel> draw(rain::detail::d3d11_frame_targets& targets,UINT width,UINT height,
                            bool use_srgb=true) {
        ID3D11RenderTargetView* view=targets.color_view(use_srgb);
        const float black[4]{};
        context.p->ClearRenderTargetView(view,black);
        context.p->ClearDepthStencilView(targets.depth_view(),D3D11_CLEAR_DEPTH,1,0);
        context.p->OMSetRenderTargets(1,&view,targets.depth_view());
        context.p->OMSetDepthStencilState(depth.p,0);
        context.p->OMSetBlendState(nullptr,nullptr,0xffffffff);
        const D3D11_VIEWPORT viewport{0,0,static_cast<float>(width),static_cast<float>(height),0,1};
        context.p->RSSetViewports(1,&viewport); context.p->RSSetState(rasterizer.p);
        context.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context.p->IASetInputLayout(layout.p);
        const UINT stride=sizeof(position),offset=0;
        context.p->IASetVertexBuffers(0,1,&vertices.p,&stride,&offset);
        context.p->VSSetShader(vs.p,nullptr,0); context.p->PSSetShader(ps.p,nullptr,0);
        context.p->VSSetConstantBuffers(0,1,&constants.p);
        context.p->PSSetConstantBuffers(0,1,&constants.p);
        std::array<float,8> params{1,1,1,1,0.2f,0,0,0};
        context.p->UpdateSubresource(constants.p,0,nullptr,params.data(),0,0);
        context.p->Draw(vertex_count,0);
        // Draw a farther red disk afterward: matched multisample depth must reject it.
        params={1,0,0,1,0.8f,0,0,0};
        context.p->UpdateSubresource(constants.p,0,nullptr,params.data(),0,0);
        context.p->Draw(vertex_count,0);

        com<ID3D11Texture2D> destination,readback;
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=width; desc.Height=height;
        desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        hr(device.p->CreateTexture2D(&desc,nullptr,destination.out()),"MSAA resolve destination");
        targets.resolve_to(context.p,destination.p);
        desc.Usage=D3D11_USAGE_STAGING; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        hr(device.p->CreateTexture2D(&desc,nullptr,readback.out()),"MSAA readback");
        context.p->CopyResource(readback.p,destination.p);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr(context.p->Map(readback.p,0,D3D11_MAP_READ,0,&mapped),"MSAA map");
        std::vector<pixel> pixels(width*height);
        for (UINT row=0;row<height;++row)
            std::memcpy(pixels.data()+row*width,
                static_cast<const char*>(mapped.pData)+row*mapped.RowPitch,width*sizeof(pixel));
        context.p->Unmap(readback.p,0);
        check(pixels[(height/2)*width+width/2].g==255,"Multisample depth did not reject farther disk");
        check(pixels.front().r==0,"Resolve background incorrect");
        for (auto p:pixels) check(p.r==p.g && p.g==p.b,"Multisample depth/color mismatch");
        return pixels;
    }

    void check_debug_messages() {
        if (!info.p) return;
        for (UINT64 i=0;i<info.p->GetNumStoredMessages();++i) {
            SIZE_T bytes=0; info.p->GetMessage(i,nullptr,&bytes);
            std::vector<unsigned char> storage(bytes);
            auto* message=reinterpret_cast<D3D11_MESSAGE*>(storage.data());
            hr(info.p->GetMessage(i,message,&bytes),"D3D debug message");
            if (message->Severity==D3D11_MESSAGE_SEVERITY_ERROR ||
                message->Severity==D3D11_MESSAGE_SEVERITY_CORRUPTION)
                throw std::runtime_error(message->pDescription);
        }
    }
};

D3D11_TEXTURE2D_DESC texture_desc(ID3D11View* view) {
    com<ID3D11Resource> resource; view->GetResource(resource.out());
    com<ID3D11Texture2D> texture;
    hr(resource.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(texture.out())),
        "View texture query");
    D3D11_TEXTURE2D_DESC desc{}; texture.p->GetDesc(&desc); return desc;
}

void check_dimensions(rain::detail::d3d11_frame_targets& targets,UINT width,UINT height) {
    const auto color=texture_desc(targets.color_view(false));
    const auto depth=texture_desc(targets.depth_view());
    check(color.Width==width && color.Height==height && depth.Width==width && depth.Height==height,
        "Framebuffer size mismatch after resize");
    check(color.SampleDesc.Count==targets.sample_count() && color.SampleDesc.Count==depth.SampleDesc.Count &&
        color.SampleDesc.Quality==depth.SampleDesc.Quality,"Color/depth sample descriptions differ");
    D3D11_RENDER_TARGET_VIEW_DESC view{}; targets.color_view(true)->GetDesc(&view);
    check(view.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,"Missing sRGB render view");
    check(view.ViewDimension==(targets.sample_count()>1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS :
        D3D11_RTV_DIMENSION_TEXTURE2D),"Wrong RTV dimension");
}

void save_ppm(const char* name,UINT width,UINT height,const std::vector<pixel>& pixels) {
    std::ofstream file(name,std::ios::binary);
    file<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for (auto p:pixels) {
        const unsigned char rgb[]={p.r,p.g,p.b};
        file.write(reinterpret_cast<const char*>(rgb),3);
    }
}
}

int main() {
    try {
        fixture gpu;
        rain::detail::d3d11_frame_targets targets;
        hr(targets.create(gpu.device.p,160,160,1),"Create single-sample reference");
        check_dimensions(targets,160,160);
        const auto before=gpu.draw(targets,160,160);
        const auto partial=[](const auto& pixels) {
            return std::count_if(pixels.begin(),pixels.end(),[](pixel p){return p.r>0 && p.r<255;});
        };
        check(partial(before)==0,"Reference must have binary coverage");

        hr(targets.create(gpu.device.p,160,160),"Create default MSAA targets");
        check_dimensions(targets,160,160);
        check(targets.sample_count()>1,"WARP did not enable MSAA");
        const UINT chosen_samples=targets.sample_count();
        const auto after=gpu.draw(targets,160,160);
        check(partial(after)>100,"MSAA did not produce smooth fractional edge coverage");
        const auto decode=[](float srgb) {
            return srgb<=0.04045f ? srgb/12.92f : std::pow((srgb+0.055f)/1.055f,2.4f);
        };
        for (auto p:after)
            check(std::abs(decode(p.r/255.0f)-p.a/255.0f)<0.009f,
                "Resolve averaged encoded sRGB values, causing dark edge halos");
        save_ppm("antialias_before.ppm",160,160,before);
        save_ppm("antialias_after.ppm",160,160,after);

        // Both views, fresh clears, resizes, lower sample budgets and copy-only fallback.
        gpu.draw(targets,160,160,false);
        for (UINT budget:{4u,3u,2u,1u}) {
            hr(targets.create(gpu.device.p,96,64,budget),"Recreate with reduced sample budget");
            check(targets.sample_count()<=budget,"Fallback exceeded requested samples");
            check_dimensions(targets,96,64); gpu.draw(targets,96,64);
        }
        ID3D11RenderTargetView* previous=targets.color_view(true);
        check(FAILED(targets.create(gpu.device.p,0,64)),"Zero-size create should fail");
        check(targets.color_view(true)==previous,"Failed resize destroyed working framebuffer");
        hr(targets.create(gpu.device.p,48,96),"Restore multisampling after resize");
        check_dimensions(targets,48,96); gpu.draw(targets,48,96);
        targets.release(); targets.release();
        check(targets.color_view(false)==nullptr && targets.depth_view()==nullptr,"Release leaked views");
        gpu.check_debug_messages();
        std::cout<<"PASS "<<chosen_samples<<"x MSAA: fractional edge pixels "<<partial(before)
                 <<" -> "<<partial(after)<<"; sRGB resolve, depth, resize and fallback verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr<<error.what()<<'\n'; return 1;
    }
}
