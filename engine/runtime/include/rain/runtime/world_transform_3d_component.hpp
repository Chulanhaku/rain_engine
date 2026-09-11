#pragma once

#include<rain/core/math/mat4.hpp>
#include<rain/core/math/vec3.hpp>

namespace rain {
	struct world_transform_3d_component {
		mat4 matrix = mat4::identity();

		vec3 position{};
		vec3 right{1.0f,0.0f,0.0f};
		vec3 up{0.0f,1.0f,0.0f};
		vec3 forward{ 0.0f,0.0f,1.0f };
	};
}