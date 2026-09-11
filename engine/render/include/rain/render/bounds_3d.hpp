#pragma once

#include<rain/core/math/vec3.hpp>
#include<rain/core/types.hpp>

namespace rain {
	struct bounding_sphere_3d {
		vec3 center{};
		f32 radius = 0.0f;
	};
}