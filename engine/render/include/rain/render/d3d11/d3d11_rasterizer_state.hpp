#pragma once

#include <rain/render/render_resource_desc.hpp>
#include <d3d11.h>

namespace rain::detail {
// Shared by production pipeline creation and offscreen D3D11 pixel regressions.
[[nodiscard]] inline D3D11_RASTERIZER_DESC make_d3d11_rasterizer_desc(const pipeline_state_desc& desc) {
    D3D11_RASTERIZER_DESC raster{};
    raster.FillMode=D3D11_FILL_SOLID;
    // CullMode names identify the faces to discard, not the faces to keep.
    switch (desc.cull_mode) {
    case render_cull_mode::none: raster.CullMode=D3D11_CULL_NONE; break;
    case render_cull_mode::front: raster.CullMode=D3D11_CULL_FRONT; break;
    case render_cull_mode::back: raster.CullMode=D3D11_CULL_BACK; break;
    }
    raster.FrontCounterClockwise=desc.front_counter_clockwise;
    raster.DepthClipEnable=TRUE;
    raster.MultisampleEnable=TRUE;
    return raster;
}
}
