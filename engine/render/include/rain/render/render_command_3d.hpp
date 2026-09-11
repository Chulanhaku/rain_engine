#pragma once

#include<rain/core/math/mat4.hpp>
#include<rain/core/math/vec4.hpp>
#include<rain/core/types.hpp>
#include<rain/render/render_handles.hpp>
#include <rain/render/render_types.hpp>
#include<rain/runtime/entity.hpp>

namespace rain {
	struct render_command_3d {
		entity_id source_entity;

		mat4 world_matrix;

		mesh_3d_handle mesh;

		material_3d_handle material;

		vec4 tint{
			1.0f,1.0f,1.0f,1.0f
		};

		f32 camera_distance_squared = 0.0f;

		i32 layer = 0;
		u64 submission_index = 0;
        f32 camera_view_depth = 0.0f;
        render_blend_mode blend_mode = render_blend_mode::opaque;
	};
}