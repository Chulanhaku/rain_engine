#pragma once

#include<rain/core/types.hpp>
#include<rain/render/render_handles.hpp>

#include<string>
#include<vector>

namespace rain {
	enum class shader_target {
		vertex_shader_4_0,
		pixel_shader_4_0
	};


	struct shader_program_desc {
		std::string name;
		std::string vertex_source;
		std::string vertex_entry = "main";
		shader_target vertex_target = shader_target::vertex_shader_4_0;

		std::string pixel_source;
		std::string pixel_entry="main";
		shader_target pixel_target = shader_target::pixel_shader_4_0;
	};

	enum class render_buffer_usage {
		immutable,
		dynamic
	};

	enum class render_buffer_bind {
		vertex_buffer,
		index_buffer
	};

	struct render_buffer_desc {
		std::string name;

		render_buffer_bind bind = render_buffer_bind::vertex_buffer;
		render_buffer_usage usage = render_buffer_usage::immutable;

		usize size_bytes = 0;
		u32 stride_bytes = 0;

		const void* initial_data = nullptr;
	};

	enum class vertex_attribute_format {
		r32g32_float,
		r32g32b32_float,
		r32g32b32a32_float
	};

	struct vertex_attribute_desc {
		std::string semantic_name;
		u32 semantic_index = 0;
		vertex_attribute_format format = vertex_attribute_format::r32g32_float;
		u32 input_slot = 0;
		u32 offset_bytes = 0;
	};

	enum class primitive_topology {
		triangle_list,
		line_list
	};

	struct pipeline_state_desc {
		std::string name;
		shader_program_handle shader;

		std::vector<vertex_attribute_desc> vertex_attributes;

		primitive_topology topology = primitive_topology::triangle_list;

	};
}