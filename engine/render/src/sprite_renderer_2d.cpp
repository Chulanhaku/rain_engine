#include <rain/render/sprite_renderer_2d.hpp>

#include <rain/core/assert.hpp>

#include <cstddef>

namespace rain
{
    namespace
    {
        constexpr const char* sprite_vertex_shader_source = R"(
struct vertex_input
{
    float2 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct vertex_output
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

vertex_output main(vertex_input input)
{
    vertex_output output;
    output.position = float4(input.position, 0.0f, 1.0f);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}
)";

        constexpr const char* sprite_pixel_shader_source = R"(
Texture2D sprite_texture : register(t0);
SamplerState sprite_sampler : register(s0);

struct pixel_input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

float4 main(pixel_input input) : SV_TARGET
{
    return sprite_texture.Sample(sprite_sampler, input.uv) * input.color;
}
)";
    }

    sprite_renderer_2d::sprite_renderer_2d(render_backend& backend, u32 max_quads)
        : backend_(&backend)
        , max_quads_(max_quads)
        , max_vertices_(max_quads * 6)
    {
        rain_assert(backend_ != nullptr);
        rain_assert(max_quads_ > 0);

        vertices_.reserve(max_vertices_);

        create_resources();
    }

    void sprite_renderer_2d::begin()
    {
        active_camera_ = nullptr;
        vertices_.clear();
        current_texture_ = default_white_texture_;
        submitted_quad_count_ = 0;
    }

    void sprite_renderer_2d::begin(const camera_2d& camera)
    {
        active_camera_ = &camera;
        vertices_.clear();
        current_texture_ = default_white_texture_;
        submitted_quad_count_ = 0;
    }

    void sprite_renderer_2d::draw_rect_ndc(
        const sprite_rect_ndc& rect,
        const sprite_color& color,
        texture_2d_handle texture,
        const sprite_uv_rect& uv,
        render_blend_mode blend_mode)
    {
        texture = resolve_texture(texture);

        const bool texture_changed = texture != current_texture_;

        const bool blend_mode_changed = blend_mode != current_blend_mode_;

        if ((texture_changed || blend_mode_changed) && !vertices_.empty())flush();

        current_texture_ = texture;
        current_blend_mode_ = blend_mode;

        const f32 left = rect.x;
        const f32 right = rect.x + rect.width;
        const f32 top = rect.y;
        const f32 bottom = rect.y - rect.height;

        push_quad_ndc(
            vec2{.x = left, .y = top},
            vec2{.x = right, .y = top},
            vec2{.x = right, .y = bottom},
            vec2{.x = left, .y = bottom},
            color,
            uv
        );
    }

    void sprite_renderer_2d::draw_rect_world(
        const sprite_rect_world& rect,
        const sprite_color& color,
        texture_2d_handle texture,
        const sprite_uv_rect& uv,
        render_blend_mode blend_mode)
    {
        rain_assert(active_camera_ != nullptr);

        texture = resolve_texture(texture);

        const bool texture_changed = texture != current_texture_;

        const bool blend_mode_changed = blend_mode != current_blend_mode_;

        if ((texture_changed || blend_mode_changed) && !vertices_.empty())flush();

        current_texture_ = texture;
        current_blend_mode_ = blend_mode;

        const simd_vec2 half_size = rect.size * 0.5f;
        const vec2 half_size_scalar = half_size.to_vec2();
        const simd_vec2 horizontal_half_size(half_size_scalar.x, 0.0f);
        const simd_vec2 vertical_half_size(0.0f, half_size_scalar.y);

        const simd_vec2 top_left_world =
            rect.center - horizontal_half_size + vertical_half_size;
        const simd_vec2 top_right_world =
            rect.center + horizontal_half_size + vertical_half_size;
        const simd_vec2 bottom_right_world =
            rect.center + horizontal_half_size - vertical_half_size;
        const simd_vec2 bottom_left_world =
            rect.center - horizontal_half_size - vertical_half_size;

        push_quad_ndc(
            active_camera_->world_to_ndc(top_left_world.to_vec2()),
            active_camera_->world_to_ndc(top_right_world.to_vec2()),
            active_camera_->world_to_ndc(bottom_right_world.to_vec2()),
            active_camera_->world_to_ndc(bottom_left_world.to_vec2()),            color,
            uv
        );
    }

    void sprite_renderer_2d::end()
    {
        flush();
    }

    u32 sprite_renderer_2d::quad_count() const
    {
        return submitted_quad_count_ + static_cast<u32>(vertices_.size() / 6);
    }

    u32 sprite_renderer_2d::vertex_count() const
    {
        return static_cast<u32>(vertices_.size());
    }

    void sprite_renderer_2d::push_quad_ndc(
        vec2 top_left,
        vec2 top_right,
        vec2 bottom_right,
        vec2 bottom_left,
        const sprite_color& color,
        const sprite_uv_rect& uv)
    {
        if (vertices_.size() + 6 > max_vertices_)
        {
            flush();
        }

        if (vertices_.size() + 6 > max_vertices_)
        {
            return;
        }

        const sprite_vertex vertex_top_left{
            .position = {top_left.x, top_left.y},
            .color = {color.r, color.g, color.b, color.a},
            .uv = {uv.min.x, uv.min.y}
        };

        const sprite_vertex vertex_top_right{
            .position = {top_right.x, top_right.y},
            .color = {color.r, color.g, color.b, color.a},
            .uv = {uv.max.x, uv.min.y}
        };

        const sprite_vertex vertex_bottom_right{
            .position = {bottom_right.x, bottom_right.y},
            .color = {color.r, color.g, color.b, color.a},
            .uv = {uv.max.x, uv.max.y}
        };

        const sprite_vertex vertex_bottom_left{
            .position = {bottom_left.x, bottom_left.y},
            .color = {color.r, color.g, color.b, color.a},
            .uv = {uv.min.x, uv.max.y}
        };

        vertices_.push_back(vertex_top_left);
        vertices_.push_back(vertex_top_right);
        vertices_.push_back(vertex_bottom_right);

        vertices_.push_back(vertex_top_left);
        vertices_.push_back(vertex_bottom_right);
        vertices_.push_back(vertex_bottom_left);
    }

    void sprite_renderer_2d::flush()
    {
        if (vertices_.empty())
        {
            return;
        }

        const usize upload_size = vertices_.size() * sizeof(sprite_vertex);

        backend_->update_buffer(
            vertex_buffer_,
            vertices_.data(),
            upload_size
        );

        backend_->set_pipeline_state(pipeline_for_blend(current_blend_mode_));
        backend_->set_vertex_buffer(vertex_buffer_);
        backend_->set_texture_2d(current_texture_, 0);
        backend_->draw(static_cast<u32>(vertices_.size()), 0);

        submitted_quad_count_ += static_cast<u32>(vertices_.size() / 6);
        vertices_.clear();
    }

    texture_2d_handle sprite_renderer_2d::resolve_texture(texture_2d_handle texture) const
    {
        if (!texture.is_valid())
        {
            return default_white_texture_;
        }

        return texture;
    }

    pipeline_state_handle sprite_renderer_2d::pipeline_for_blend(render_blend_mode blend_mode)const {
        switch (blend_mode) {
        case render_blend_mode::opaque :
            return opaque_pipeline_;
        case render_blend_mode::alpha:
            return alpha_pipeline_;
        case render_blend_mode::additive:
            return additive_pipeline_;

        }

        return alpha_pipeline_;
    }

    void sprite_renderer_2d::create_resources()
    {
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
            .size_bytes = max_vertices_ * sizeof(sprite_vertex),
            .stride_bytes = sizeof(sprite_vertex),
            .initial_data = nullptr
        });

        const std::vector<vertex_attribute_desc> vertex_attributes = {
            vertex_attribute_desc{
                .semantic_name = "POSITION",
                .semantic_index = 0,
                .format = vertex_attribute_format::r32g32_float,
                .input_slot = 0,
                .offset_bytes = offsetof(sprite_vertex, position)
            },
            vertex_attribute_desc{
                .semantic_name = "COLOR",
                .semantic_index = 0,
                .format = vertex_attribute_format::r32g32b32a32_float,
                .input_slot = 0,
                .offset_bytes = offsetof(sprite_vertex, color)
            },
            vertex_attribute_desc{
                .semantic_name = "TEXCOORD",
                .semantic_index = 0,
                .format = vertex_attribute_format::r32g32_float,
                .input_slot = 0,
                .offset_bytes = offsetof(sprite_vertex, uv)
            }
        };

        opaque_pipeline_ = backend_->create_pipeline_state(
            pipeline_state_desc{
                .name = "sprite_2d_opaque_pipeline",
                .shader = shader_,
                .vertex_attributes = vertex_attributes,
                .topology = primitive_topology::triangle_list,
                .blend_mode = render_blend_mode::opaque
            }
        );

        alpha_pipeline_ = backend_->create_pipeline_state(
            pipeline_state_desc{
                .name = "sprite_2d_alpha_pipeline",
                .shader = shader_,
                .vertex_attributes = vertex_attributes,
                .topology = primitive_topology::triangle_list,
                .blend_mode = render_blend_mode::alpha
            }
        );

        additive_pipeline_ = backend_->create_pipeline_state(
            pipeline_state_desc{
                .name = "sprite_2d_additive_pipeline",
                .shader = shader_,
                .vertex_attributes = vertex_attributes,
                .topology = primitive_topology::triangle_list,
                .blend_mode = render_blend_mode::additive
            }
        );

        const u8 white_pixel[4] = {
            255,
            255,
            255,
            255
        };

        default_white_texture_ = backend_->create_texture_2d(texture_2d_desc{
            .name = "default_white_texture",
            .width = 1,
            .height = 1,
            .format = texture_format::rgba8_unorm,
            .pixels = white_pixel,
            .size_bytes = sizeof(white_pixel)
        });

        current_texture_ = default_white_texture_;
    }
}