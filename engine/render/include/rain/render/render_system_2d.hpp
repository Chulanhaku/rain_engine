#pragma once

#include <rain/core/types.hpp>
#include <rain/render/camera_2d.hpp>
#include <rain/render/material_2d_registry.hpp>
#include <rain/render/render_queue_2d.hpp>
#include <rain/render/sprite_renderer_2d.hpp>
#include <rain/runtime/system_scheduler.hpp>
#include <rain/runtime/world.hpp>

namespace rain {
    class render_system_2d {
    public:
        explicit render_system_2d(
            render_backend& backend,
            material_2d_registry& materials,
            u32 max_quads = 4096,
            u32 max_commands = 4096
        );

        render_system_2d(const render_system_2d&) = delete;
        render_system_2d& operator=(const render_system_2d&) = delete;

        void prepare(world& target_world, const entity_query_desc& query);
        void submit(const camera_2d& camera);
        void clear();

        [[nodiscard]] static entity_query_desc make_entity_query();
        [[nodiscard]] const render_queue_2d& queue() const;
        [[nodiscard]] u32 last_quad_count() const;
        [[nodiscard]] u32 last_command_count() const;

        void render(world& target_world, const camera_2d& camera);
        void set_entity_query(const entity_query_desc& desc);
        [[nodiscard]] const entity_query_desc& entity_query() const;

    private:
        sprite_renderer_2d sprite_renderer_;
        material_2d_registry* materials_ = nullptr;
        render_queue_2d queue_;
        entity_query_desc entity_query_;
        u32 last_quad_count_ = 0;
        u32 last_command_count_ = 0;
    };

    void render_prepare_system_2d(system_context& context, void* user_data);
}
