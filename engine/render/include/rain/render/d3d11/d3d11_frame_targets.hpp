#pragma once

#include <d3d11.h>

namespace rain::detail {
    // Owns the complete color/depth sample set so their dimensions and sample
    // descriptions cannot diverge on resize. Both 2D and 3D use this framebuffer.
    class d3d11_frame_targets {
    public:
        d3d11_frame_targets() = default;
        ~d3d11_frame_targets();
        d3d11_frame_targets(const d3d11_frame_targets&) = delete;
        d3d11_frame_targets& operator=(const d3d11_frame_targets&) = delete;

        // Prefer 8x; fall back to 4x, 2x, then 1x. Existing targets survive failure.
        [[nodiscard]] HRESULT create(ID3D11Device* device, UINT width, UINT height,
                                     UINT max_samples = 8);
        void release();
        void resolve_to(ID3D11DeviceContext* context, ID3D11Texture2D* destination) const;

        [[nodiscard]] UINT sample_count() const { return sample_desc_.Count; }
        [[nodiscard]] ID3D11RenderTargetView* color_view(bool srgb) const {
            return srgb ? srgb_view_ : linear_view_;
        }
        [[nodiscard]] ID3D11DepthStencilView* depth_view() const { return depth_view_; }

    private:
        [[nodiscard]] HRESULT create_sample_set(ID3D11Device* device, UINT width,
                                               UINT height, UINT samples);
        void swap(d3d11_frame_targets& other);

        DXGI_SAMPLE_DESC sample_desc_{1, 0};
        ID3D11Texture2D* color_ = nullptr;
        ID3D11Texture2D* resolved_ = nullptr;
        ID3D11Texture2D* depth_ = nullptr;
        ID3D11RenderTargetView* linear_view_ = nullptr;
        ID3D11RenderTargetView* srgb_view_ = nullptr;
        ID3D11DepthStencilView* depth_view_ = nullptr;
    };
}
