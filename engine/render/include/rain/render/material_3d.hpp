#pragma once

#include<rain/core/math/vec4.hpp>
#include<rain/render/render_handles.hpp>
#include<rain/render/render_types.hpp>
#include<rain/core/types.hpp>

#include<string>

namespace rain {
	struct material_3d_desc {
		std::string name;
		texture_2d_handle albedo_texture{};

        // Linear RGB; alpha is linear coverage.
		vec4 base_color{
			1.0f,
			1.0f,
			1.0f,
			1.0f
		};

		f32 metallic_factor = 0.0f;
		f32 roughness_factor = 1.0f;

		f32 alpha_cutoff = -1.0f;

        render_blend_mode blend_mode = render_blend_mode::opaque;

        // Linear data texture: green = roughness, blue = metallic.
        texture_2d_handle metallic_roughness_texture{};
        bool double_sided = false;
        // Set false when albedo uses a hardware sRGB view or contains linear data.
        bool albedo_manual_srgb_decode = true;

	};

	using material_3d = material_3d_desc;
}