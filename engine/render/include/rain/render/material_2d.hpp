#pragma once

#include <rain/core/types.hpp>
#include<rain/render/render_handles.hpp>
#include<rain/render/render_types.hpp>

#include<string>

namespace rain {

	struct material_2d_desc {
		std::string name;

		texture_2d_handle texture;
		render_blend_mode blend_mode = render_blend_mode::alpha;

	};

	struct material_2d {
		std::string name;

		texture_2d_handle texture;
		render_blend_mode blend_mode = render_blend_mode::alpha;
	};
}