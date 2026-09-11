#pragma once

#include<rain/core/math/vec2.hpp>
#include<rain/core/math/vec3.hpp>
#include<rain/core/types.hpp>
#include<rain/render/render_handles.hpp>
#include<rain/render/bounds_3d.hpp>

#include<span>
#include<string>

namespace rain {
	struct mesh_vertex_3d {
		vec3 position;
		vec3 normal;
		vec2 uv;
	};

	struct mesh_3d_desc {
		std::string name;
		std::span<const mesh_vertex_3d>vertices;
		std::span<const u32>indices;

	};

	struct mesh_3d {
		std::string name;
		render_buffer_handle vertex_buffer;
		render_buffer_handle index_buffer;
		u32 index_count = 0;

		bounding_sphere_3d local_bounds;
	};


}