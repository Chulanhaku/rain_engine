#pragma once

#include<rain/core/types.hpp>
#include<rain/render/render_backend.hpp>
#include<rain?render/render_handles.hpp>

#include<vector>

namespace rain {
	struct sprite_color {
		f32 r = 1.0f;
		f32 b = 1.0f;
		f32 g = 1.0f;
		f32 a = 1.0f;
	};

	struct sprite_rect {
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 width = 0.0f;
		f32 height = 0.0f;
	};
}