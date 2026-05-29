#pragma once

#include<rain/core/types.hpp>
#include<rain/render/render_clear_color.hpp>
#include<rain/render/render_resource_desc.hpp>
#include<rain/render/render_handles.hpp>

namespace rain {
	class render_backend {
	public:
		virtual~render_backend() = default;
		virtual void begin_frame() = 0;
		virtual void clear(const render_clear_color& color) = 0;
		virtual void draw_debug_triangle() = 0;
		virtual void end_frame() = 0;

		virtual void resize(u32 width,u32 height) = 0;

		[[nodiscard]] virtual shader_program_handle create_shader_program(const shader_program_desc& desc) = 0;

		[[nodiscard]] virtual render_buffer_handle create_vertex_buffer(const render_buffer_desc& desc) = 0;

		[[nodiscard]] virtual pipeline_state_handle create_pipeline_state(const pipeline_state_desc&desc) = 0;

		virtual void set_pipeline_state(pipeline_state_handle handle) = 0;
		virtual void set_vertex_buffer(render_buffer_handle handle) = 0;
		virtual void draw(u32 vertex_count,u32 start_vertex) = 0;
		virtual void update_buffer(render_buffer_handle handle, const void* data, usize size_bytes) = 0;


		[[nodiscard]] virtual u32 width()const = 0;
		[[nodiscard]] virtual u32 height()const = 0;
	};
}