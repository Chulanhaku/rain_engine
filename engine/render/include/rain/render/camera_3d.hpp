#pragma once

#include<rain/core/types.hpp>
#include<rain/render/render_command_3d.hpp>


namespace rain {
	struct camera_3d_component {
		f32 vertical_fov_radians = 1.04719755f;
		f32 near_plane = 0.1f;
		f32 far_plane = 1000.0f;
	};



}