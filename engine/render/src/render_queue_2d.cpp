#include <rain/render/render_queue_2d.hpp>

#include <algorithm>
#include <utility>

namespace rain {
    render_queue_2d::render_queue_2d(u32 initial_capacity) {
        commands_.reserve(initial_capacity);
    }

    void render_queue_2d::begin_frame() {
        commands_.clear();
        stats_ = render_queue_2d_stats{};
        next_submission_index_ = 0;
    }

    void render_queue_2d::push(render_command_2d command) {
        command.submission_index = next_submission_index_++;
        commands_.push_back(std::move(command));
        ++stats_.command_count;
    }

    void render_queue_2d::sort(const material_2d_registry& materials) {
        stats_.opaque_count = 0;
        stats_.alpha_count = 0;
        stats_.additive_count = 0;

        for (const render_command_2d& command : commands_) {
            switch (resolve_blend_mode(materials, command.material)) {
            case render_blend_mode::opaque:
                ++stats_.opaque_count;
                break;
            case render_blend_mode::alpha:
                ++stats_.alpha_count;
                break;
            case render_blend_mode::additive:
                ++stats_.additive_count;
                break;
            }
        }

        std::stable_sort(
            commands_.begin(),
            commands_.end(),
            [&materials](const render_command_2d& lhs, const render_command_2d& rhs) {
                if (lhs.layer != rhs.layer) {
                    return lhs.layer < rhs.layer;
                }
                if (lhs.order_in_layer != rhs.order_in_layer) {
                    return lhs.order_in_layer < rhs.order_in_layer;
                }

                const render_blend_mode lhs_blend =
                    resolve_blend_mode(materials, lhs.material);
                const render_blend_mode rhs_blend =
                    resolve_blend_mode(materials, rhs.material);
                const u32 lhs_bucket = blend_bucket(lhs_blend);
                const u32 rhs_bucket = blend_bucket(rhs_blend);

                if (lhs_bucket != rhs_bucket) {
                    return lhs_bucket < rhs_bucket;
                }

                if (lhs_blend == render_blend_mode::opaque) {
                    if (lhs.material.index != rhs.material.index) {
                        return lhs.material.index < rhs.material.index;
                    }
                    if (lhs.material.generation != rhs.material.generation) {
                        return lhs.material.generation < rhs.material.generation;
                    }
                }

                return lhs.submission_index < rhs.submission_index;
            }
        );
    }

    std::span<const render_command_2d> render_queue_2d::commands() const {
        return commands_;
    }

    const render_queue_2d_stats& render_queue_2d::stats() const {
        return stats_;
    }

    bool render_queue_2d::empty() const {
        return commands_.empty();
    }

    usize render_queue_2d::size() const {
        return commands_.size();
    }

    render_blend_mode render_queue_2d::resolve_blend_mode(
        const material_2d_registry& materials,
        material_2d_handle handle
    ) {
        const material_2d* material = materials.try_get(handle);
        if (material == nullptr) {
            material = materials.try_get(materials.default_material());
        }
        return material == nullptr ? render_blend_mode::alpha : material->blend_mode;
    }

    u32 render_queue_2d::blend_bucket(render_blend_mode mode) {
        switch (mode) {
        case render_blend_mode::opaque:
            return 0;
        case render_blend_mode::alpha:
            return 1;
        case render_blend_mode::additive:
            return 2;
        }
        return 3;
    }
}
