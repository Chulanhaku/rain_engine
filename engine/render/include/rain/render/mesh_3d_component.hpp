#pragma once

#include<rain/core/math/vec4.hpp>
#include<rain/core/types.hpp>
#include<rain/render/render_handles.hpp>

namespace rain {
	struct mesh_3d_component {
		mesh_3d_handle mesh;
		material_3d_handle material;

		vec4 tint{
			1.0f,
			1.0f,
			1.0f,
			1.0f,
		};

		i32 layer = 0;
	};
}