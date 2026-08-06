#pragma once

#include<rain/render/render_command_3d.hpp>
#include<rain/runtime/transform_3d_component.hpp>
#include<rain/runtime/rotation_system_3d.hpp>
#include<rain/render/camera_3d.hpp>
#include<rain/render/mesh_3d_component.hpp>

namespace rain {

	struct camera_3d_frame {
		mat4 view;
		mat4 projection;
		mat4 view_projection;

		vec3 position;
	};


	class render_system_3d {
	public:
		entity_query_desc make_camera_query();

		entity_query_desc make_render_query();

		entity_query_desc make_light_query();

		void prepare(world& target_world);
	};


	void render_prepare_system_3d(system_context& context, void* user_data);
}