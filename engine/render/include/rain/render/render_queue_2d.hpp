#pragma once

#include <rain/core/types.hpp>
#include <rain/render/material_2d_registry.hpp>
#include <rain/render/render_command_2d.hpp>

#include <span>
#include <vector>

namespace rain {
    struct render_queue_2d_stats {
        u32 command_count = 0;
        u32 opaque_count = 0;
        u32 alpha_count = 0;
        u32 additive_count = 0;
    };

    class render_queue_2d {
    public:
        explicit render_queue_2d(u32 initial_capacity = 4096);

        void begin_frame();
        void push(render_command_2d command);
        void sort(const material_2d_registry& materials);

        [[nodiscard]] std::span<const render_command_2d> commands() const;
        [[nodiscard]] const render_queue_2d_stats& stats() const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] usize size() const;

    private:
        [[nodiscard]] static render_blend_mode resolve_blend_mode(
            const material_2d_registry& materials,
            material_2d_handle handle
        );
        [[nodiscard]] static u32 blend_bucket(render_blend_mode mode);

        std::vector<render_command_2d> commands_;
        render_queue_2d_stats stats_;
        u64 next_submission_index_ = 0;
    };
}
