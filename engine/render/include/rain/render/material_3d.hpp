#pragma once

#include<rain/core/math/vec4.hpp>
#include<rain/render/render_handles.hpp>
#include<rain/render/render_types.hpp>

#include<string>

namespace rain {
	struct material_3d_desc {
		std::string name;
		texture_2d_handle albedo_texture;

		vec4 base_color{
			1.0f,
			1.0f,
			1.0f,
			1.0f
		};

		render_blend_mode blend_mode = render_blend_mode::opaque;

	};

	using material_3d = material_3d_desc;
}