#pragma once

#include<rain/core/math/vec3.hpp>
#include<rain/core/types.hpp>

namespace rain{
	struct directional_light_3d_component {
		vec3 direction{
			0.4f,
			-1.0f,
			0.3f
		};
		vec3 color{
			1.0f,
			1.0f,
			1.0f
		};

		f32 intensity = 1.0f;

		vec3 ambient_color{
			0.12f,
			0.14f,
			0.18f
		};

	};
}