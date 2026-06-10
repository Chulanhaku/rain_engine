#pragma once

#include<rain/core/types.hpp>
#include<rain/render/render_backend.hpp>
#include<rain/render/render_handles.hpp>
#include<rain/core/math/vec2.hpp>
#include<rain/render/camera_2d.hpp>

#include<vector>

namespace rain {
	struct sprite_color {
		f32 r = 1.0f;
		f32 g = 1.0f;
		f32 b = 1.0f;
		f32 a = 1.0f;
	};

	struct sprite_rect_ndc {
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 width = 0.0f;
		f32 height = 0.0f;

	};

	struct sprite_rect_world {
		vec2 center{ 0.0f, 0.0f };
		vec2 size{100.0f, 100.0f};
	};

	class sprite_renderer_2d {
	public:
		explicit sprite_renderer_2d(render_backend& backend, u32 max_quads = 1024);

		sprite_renderer_2d(const sprite_renderer_2d&)=delete;
		sprite_renderer_2d& operator=(const sprite_renderer_2d&) = delete;

		void begin();
		void begin(const camera_2d&camera);

		void draw_rect_ndc(const sprite_rect_ndc& rect, const sprite_color& color);
		void draw_rect_world(const sprite_rect_world& rect, const sprite_color& color);

		void end();

		[[nodiscard]] u32 quad_count()const;
		[[nodiscard]] u32 vertex_count()const;

	private:
		struct sprite_vertex {
			f32 position[2];
			f32 color[4];
		};

		void create_resources();

		void push_quad_ndc(vec2 top_left, vec2 top_right, vec2 bottom_right, vec2 bottom_left, const sprite_color& color);

	private:
		render_backend* backend_ = nullptr;

		const camera_2d* active_camera_ = nullptr;

		u32 max_quads_ = 0;
		u32 max_vertices_ = 0;

		std::vector<sprite_vertex>vertices_;

		shader_program_handle shader_;
		render_buffer_handle vertex_buffer_;
		pipeline_state_handle pipeline_;
	};
}