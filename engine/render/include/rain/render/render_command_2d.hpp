#pragma once

#include <rain/core/math/simd_vec2.hpp>
#include <rain/core/types.hpp>
#include <rain/render/render_handles.hpp>
#include <rain/render/sprite_renderer_2d.hpp>
#include <rain/runtime/entity.hpp>

namespace rain {
    struct render_command_2d {
        entity_id source_entity;
        simd_vec2 center{0.0f, 0.0f};
        simd_vec2 size{100.0f, 100.0f};
        material_2d_handle material;
        sprite_color tint{1.0f, 1.0f, 1.0f, 1.0f};
        sprite_uv_rect uv{};
        i32 layer = 0;
        i32 order_in_layer = 0;
        u64 submission_index = 0;
    };
}
