#pragma once

#include<rain/core/math/mat4.hpp>
#include<rain/core/math/vec3.hpp>

namespace rain {
	struct transform_3d_component {
		vec3 position{ 0.0f,0.0f,0.0f };
		vec3 rotation{ 0.0f,0.0f,0.0f };
		vec3 scale{ 1.0f,1.0f,1.0f };

		[[nodiscard]] mat4 matrix()const {
			return make_transform(position,rotation,scale)
		}
	};
}