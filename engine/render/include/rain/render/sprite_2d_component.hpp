#pragma once

#include<rain/core/math/vec2.hpp>

#include<rain/render/sprite_renderer_2d.hpp>

namespace rain {
	struct sprite_2d_component {
		vec2 size{ 100.0f,100.0f };
		sprite_color color{ 1.0f,1.0f,1.0f,1.0f };

		bool visible = true;
	};
}