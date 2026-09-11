#pragma once

#include<rain/core/math/mat4.hpp>

namespace rain {
	struct local_matrix_3d_component {
		mat4 matrix = mat4::identity();
	};
}