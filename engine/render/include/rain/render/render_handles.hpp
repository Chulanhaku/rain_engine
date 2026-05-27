#pragma once

#include<rain/core/handle.hpp>

namespace rain {
	struct shader_program_handle_tag {

	};

	struct render_buffer_handle_tag {

	};

	struct pipeline_state_handle_tag {


	};

	using shader_program_handle = handle<shader_program_handle_tag>;
	using render_buffer_handle = handle<render_buffer_handle_tag>;
	using pipeline_state_handle = handle<pipeline_state_handle_tag>;
}
