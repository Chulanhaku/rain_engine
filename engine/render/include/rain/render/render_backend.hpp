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
		[[nodiscard]] virtual texture_2d_handle create_texture_2d(const texture_2d_desc& desc) = 0;

		virtual bool destroy_shader_program(shader_program_handle handle) = 0;
		virtual bool destroy_render_buffer(render_buffer_handle handle) = 0;
		virtual bool destroy_pipeline_state(pipeline_state_handle handle) = 0;
		virtual bool destroy_texture_2d(texture_2d_handle handle) = 0;
		
        [[nodiscard]] virtual bool is_valid(
            shader_program_handle handle) const = 0;

        [[nodiscard]] virtual bool is_valid(
            render_buffer_handle handle) const = 0;

        [[nodiscard]] virtual bool is_valid(
            pipeline_state_handle handle) const = 0;

        [[nodiscard]] virtual bool is_valid(
            texture_2d_handle handle) const = 0;

        virtual void flush_resource_destruction() = 0;

		virtual void set_pipeline_state(pipeline_state_handle handle) = 0;
		virtual void set_vertex_buffer(render_buffer_handle handle) = 0;
		virtual void draw(u32 vertex_count,u32 start_vertex) = 0;
		virtual void update_buffer(render_buffer_handle handle, const void* data, usize size_bytes) = 0;
		virtual void set_texture_2d(texture_2d_handle handle, u32 slot) = 0;

		[[nodiscard]] virtual u32 width()const = 0;
		[[nodiscard]] virtual u32 height()const = 0;

		virtual void clear_depth(f32 depth = 1.0f) = 0;

		[[nodiscard]] virtual render_buffer_handle create_index_buffer(const render_buffer_desc& desc) = 0;
		[[nodiscard]] virtual render_buffer_handle create_constant_buffer(const render_buffer_desc& desc) = 0;

		virtual void set_index_buffer(render_buffer_handle handle, render_index_format format) = 0;
		virtual void set_vertex_buffer(render_buffer_handle handle, u32 slot) = 0;

		virtual void set_vertex_constant_buffer(render_buffer_handle handle, u32 slot) = 0;
		virtual void set_pixel_constant_buffer(render_buffer_handle handle, u32 slot) = 0;
		virtual void draw_indexed(u32 index_count,u32 start_index,i32 base_vertex)=0;
	};
}