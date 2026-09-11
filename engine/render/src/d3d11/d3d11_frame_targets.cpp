#include <rain/render/d3d11/d3d11_frame_targets.hpp>

#include <utility>

namespace rain::detail {
    namespace {
        template<class T> void release_com(T*& resource) {
            if (resource != nullptr) { resource->Release(); resource = nullptr; }
        }

        bool supports_samples(ID3D11Device* device, UINT samples) {
            if (samples == 1) return true;
            UINT support = 0;
            if (FAILED(device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, &support)) ||
                (support & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE) == 0) return false;
            for (DXGI_FORMAT format : {DXGI_FORMAT_R8G8B8A8_UNORM,
                                       DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                       DXGI_FORMAT_D24_UNORM_S8_UINT}) {
                UINT levels = 0;
                if (FAILED(device->CheckMultisampleQualityLevels(format, samples, &levels)) ||
                    levels == 0) return false;
            }
            return true;
        }
    }

    d3d11_frame_targets::~d3d11_frame_targets() { release(); }

    void d3d11_frame_targets::release() {
        release_com(depth_view_);
        release_com(srgb_view_);
        release_com(linear_view_);
        release_com(depth_);
        release_com(resolved_);
        release_com(color_);
        sample_desc_ = {1, 0};
    }

    void d3d11_frame_targets::swap(d3d11_frame_targets& other) {
        std::swap(sample_desc_, other.sample_desc_);
        std::swap(color_, other.color_);
        std::swap(resolved_, other.resolved_);
        std::swap(depth_, other.depth_);
        std::swap(linear_view_, other.linear_view_);
        std::swap(srgb_view_, other.srgb_view_);
        std::swap(depth_view_, other.depth_view_);
    }

    HRESULT d3d11_frame_targets::create(ID3D11Device* device, UINT width, UINT height,
                                       UINT max_samples) {
        if (device == nullptr || width == 0 || height == 0 || max_samples == 0)
            return E_INVALIDARG;
        HRESULT result = E_FAIL;
        for (UINT samples : {8u, 4u, 2u, 1u}) {
            if (samples > max_samples || !supports_samples(device, samples)) continue;
            d3d11_frame_targets candidate;
            result = candidate.create_sample_set(device, width, height, samples);
            if (SUCCEEDED(result)) {
                swap(candidate);
                return result;
            }
        }
        return result;
    }

    HRESULT d3d11_frame_targets::create_sample_set(ID3D11Device* device, UINT width,
                                                  UINT height, UINT samples) {
        sample_desc_ = {samples, 0};
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width; desc.Height = height;
        desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        desc.SampleDesc = sample_desc_;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET;
        HRESULT result = device->CreateTexture2D(&desc, nullptr, &color_);
        if (FAILED(result)) return result;

        D3D11_RENDER_TARGET_VIEW_DESC view{};
        view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        view.ViewDimension = samples > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS
                                        : D3D11_RTV_DIMENSION_TEXTURE2D;
        result = device->CreateRenderTargetView(color_, &view, &linear_view_);
        if (FAILED(result)) return result;
        view.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        result = device->CreateRenderTargetView(color_, &view, &srgb_view_);
        if (FAILED(result)) return result;

        if (samples > 1) {
            // A typeless intermediate permits a typed sRGB resolve. Resolving
            // directly to the typed UNORM swap-chain texture would require an
            // UNORM resolve, which averages encoded values and darkens edges.
            desc.SampleDesc = {1, 0};
            desc.BindFlags = 0;
            result = device->CreateTexture2D(&desc, nullptr, &resolved_);
            if (FAILED(result)) return result;
        }

        desc.SampleDesc = sample_desc_;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        result = device->CreateTexture2D(&desc, nullptr, &depth_);
        if (FAILED(result)) return result;
        D3D11_DEPTH_STENCIL_VIEW_DESC depth_view{};
        depth_view.Format = desc.Format;
        depth_view.ViewDimension = samples > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                              : D3D11_DSV_DIMENSION_TEXTURE2D;
        return device->CreateDepthStencilView(depth_, &depth_view, &depth_view_);
    }

    void d3d11_frame_targets::resolve_to(ID3D11DeviceContext* context,
                                       ID3D11Texture2D* destination) const {
        if (context == nullptr || destination == nullptr || color_ == nullptr) return;
        // Unbind writes before resolving/copying. The next frame's pipeline
        // binds both its color view and the matching multisampled depth target.
        context->OMSetRenderTargets(0, nullptr, nullptr);
        if (sample_desc_.Count > 1) {
            context->ResolveSubresource(resolved_, 0, color_, 0,
                                        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
            context->CopyResource(destination, resolved_);
        } else {
            context->CopyResource(destination, color_);
        }
    }
}
