#include <rain/render/sprite_renderer_2d.hpp>

#include<rain/core/assert.hpp>

#include<cstddef>

namespace rain {
	namespace {
		constexpr const char* sprite_vertex_shader_source = R"(
struct vertex_input{
	float2 position :POSITION;
	float4 color:COLOR;
};

struct vertex_output{
	float4 position:SV_POSITION;
	float4 color :COLOR;
};

vertex_output main(vertex_input input){
	vertex_output output;
	output.position = float4(input.position,0.0f,1.0f);
	output.color = input.color;
	return output;
}

)";

		constexpr const char* sprite_pixel_shader_source = R"(
struct pixel_input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(pixel_input input) : SV_TARGET
{
    return input.color;
}
)";
	}

	sprite_renderer_2d::sprite_renderer_2d(render_backend& backend, u32 max_quads) :backend_(&backend), max_quads_(max_quads), max_vertices_(max_quads * 6) {
		rain_assert(backend_ != nullptr);
		rain_assert(max_quads_ > 0);

		vertices_.reserve(max_vertices_);
		create_resources();
	}

	void sprite_renderer_2d::begin() {
		active_camera_ = nullptr;
		vertices_.clear();
	}

	void sprite_renderer_2d::begin(const camera_2d&camera) {
		active_camera_ = &camera;
		vertices_.clear();
	}


	void sprite_renderer_2d::draw_rect_ndc(const sprite_rect_ndc& rect, const sprite_color& color) {
		if (vertices_.size() + 6 > max_vertices_)return;

		const f32 left = rect.x;
		const f32 right = rect.x + rect.width;
		const f32 top = rect.y;
		const f32 bottom = rect.y - rect.height;

		push_quad_ndc(
			vec2{ .x = left, .y = top },
			vec2{ .x = right, .y = top },
			vec2{ .x = right, .y = bottom },
			vec2{ .x = left, .y = bottom },
			color
		);

		//const sprite_vertex top_left{
		//	.position = {left,top},
		//	.color = {color.r,color.g,color.b,color.a}
		//};

		//const sprite_vertex top_right{
		//	.position = {right,top},
		//	.color = {color.r,color.g,color.b,color.a}
		//};

		//const sprite_vertex bottom_right{
		//	.position = {right,bottom},
		//	.color = {color.r,color.g,color.b,color.a}
		//};

		//const sprite_vertex bottom_left{
		//	.position = {left,bottom},
		//	.color = {color.r,color.g,color.b,color.a}
		//};

		//vertices_.push_back(top_left);
		//vertices_.push_back(top_right);
		//vertices_.push_back(bottom_right);

		//vertices_.push_back(top_left);
		//vertices_.push_back(bottom_right);
		//vertices_.push_back(bottom_left);
	}

	void sprite_renderer_2d::draw_rect_world(
		const sprite_rect_world& rect,
		const sprite_color& color)
	{
		rain_assert(active_camera_ != nullptr);

		const f32 half_width = rect.size.x * 0.5f;
		const f32 half_height = rect.size.y * 0.5f;

		const vec2 top_left_world{
			.x = rect.center.x - half_width,
			.y = rect.center.y + half_height
		};

		const vec2 top_right_world{
			.x = rect.center.x + half_width,
			.y = rect.center.y + half_height
		};

		const vec2 bottom_right_world{
			.x = rect.center.x + half_width,
			.y = rect.center.y - half_height
		};

		const vec2 bottom_left_world{
			.x = rect.center.x - half_width,
			.y = rect.center.y - half_height
		};

		push_quad_ndc(
			active_camera_->world_to_ndc(top_left_world),
			active_camera_->world_to_ndc(top_right_world),
			active_camera_->world_to_ndc(bottom_right_world),
			active_camera_->world_to_ndc(bottom_left_world),
			color
		);
	}


	void sprite_renderer_2d::end() {
		if (vertices_.empty())return;

		const usize upload_size = vertices_.size() * sizeof(sprite_vertex);

		backend_->update_buffer(vertex_buffer_,vertices_.data(),upload_size);

		backend_->set_pipeline_state(pipeline_);
		backend_->set_vertex_buffer(vertex_buffer_);
		backend_->draw(static_cast<u32>(vertices_.size()), 0);
	}

	u32 sprite_renderer_2d::quad_count()const {
		return static_cast<u32>(vertices_.size() / 6);
	}

	u32 sprite_renderer_2d::vertex_count()const {
		return static_cast<u32>(vertices_.size());
	}

	void sprite_renderer_2d::push_quad_ndc(vec2 top_left, vec2 top_right, vec2 bottom_right, vec2 bottom_left, const sprite_color& color) {
		if (vertices_.size() + 6 > max_vertices_)return;

		const sprite_vertex vertex_top_left{
			.position = {top_left.x,top_left.y},
			.color = {color.r,color.g,color.b,color.a}
		};

		const sprite_vertex vertex_top_right{
			.position = {top_right.x,top_right.y},
			.color = {color.r,color.g,color.b,color.a}
		};

		const sprite_vertex vertex_bottom_right{
			.position = {bottom_right.x,bottom_right.y},
			.color = {color.r,color.g,color.b,color.a}
		};

		const sprite_vertex vertex_bottom_left{
			.position = {bottom_left.x,bottom_left.y},
			.color = {color.r,color.g,color.b,color.a}
		};

		vertices_.push_back(vertex_top_left);
		vertices_.push_back(vertex_top_right);
		vertices_.push_back(vertex_bottom_right);

		vertices_.push_back(vertex_top_left);
		vertices_.push_back(vertex_bottom_right);
		vertices_.push_back(vertex_bottom_left);

	}

	void sprite_renderer_2d::create_resources() {
		shader_ = backend_->create_shader_program(shader_program_desc{
			.name = "sprite_2d_shader",
			.vertex_source = sprite_vertex_shader_source,
			.vertex_entry = "main",
			.vertex_target = shader_target::vertex_shader_4_0,
			.pixel_source = sprite_pixel_shader_source,
			.pixel_entry = "main",
			.pixel_target = shader_target::pixel_shader_4_0
		});

		vertex_buffer_ = backend_->create_vertex_buffer(render_buffer_desc{
			.name = "sprite_2d_dynamic_vertex_buffer",
			.bind = render_buffer_bind::vertex_buffer,
			.usage = render_buffer_usage::dynamic,
			.size_bytes = max_vertices_*sizeof(sprite_vertex),
			.stride_bytes = sizeof(sprite_vertex),
			.initial_data = nullptr
		});

		pipeline_ = backend_-> create_pipeline_state(pipeline_state_desc{
			.name = "sprite_2d_pipeline",
			.shader = shader_,
			.vertex_attributes = {
				vertex_attribute_desc{
					.semantic_name = "POSITION",
					.semantic_index = 0,
					.format = vertex_attribute_format::r32g32_float,
					.input_slot = 0,
					.offset_bytes = offsetof(sprite_vertex,position)
				},
				vertex_attribute_desc{
					.semantic_name = "COLOR",
					.semantic_index = 0,
					.format = vertex_attribute_format::r32g32b32a32_float,
					.input_slot = 0,
					.offset_bytes = offsetof(sprite_vertex,color)
				}
			},
			.topology = primitive_topology::triangle_list
		});
	}
}