#pragma once

#include <rain/render/camera_3d.hpp>
#include <rain/render/directional_light_3d_component.hpp>
#include <rain/render/material_3d_registry.hpp>
#include <rain/render/mesh_3d_component.hpp>
#include <rain/render/mesh_3d_registry.hpp>
#include <rain/render/render_command_3d.hpp>
#include <rain/runtime/system_scheduler.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/world.hpp>

#include <vector>

namespace rain {
    struct render_system_3d_stats {
        u32 candidate_count = 0;
        u32 visible_count = 0;
        u32 culled_count = 0;
    };

    class render_system_3d {
    public:
        render_system_3d(render_backend& backend, mesh_3d_registry& meshes, material_3d_registry& materials);
        ~render_system_3d();

        render_system_3d(const render_system_3d&) = delete;
        render_system_3d& operator=(const render_system_3d&) = delete;

        void prepare(world& target_world);
        void submit();
        void clear();

        [[nodiscard]] static entity_query_desc make_camera_query();
        [[nodiscard]] static entity_query_desc make_render_query();
        [[nodiscard]] static entity_query_desc make_light_query();
        [[nodiscard]] usize command_count() const;

        [[nodiscard]] const render_system_3d_stats& stats()const {
            return stats_;
        }

    private:
        render_backend* backend_ = nullptr;
        mesh_3d_registry* meshes_ = nullptr;
        material_3d_registry* materials_ = nullptr;
        entity_query_desc camera_query_;
        entity_query_desc render_query_;
        entity_query_desc light_query_;
        std::vector<render_command_3d> commands_;
        camera_3d_frame camera_frame_{};
        directional_light_3d_component light_{};
        shader_program_handle shader_;
        pipeline_state_handle pipelines_[3][2][2]{};
        render_buffer_handle object_constants_;
        render_buffer_handle scene_constants_;
        render_buffer_handle material_constants_;
        bool has_camera_ = false;
        render_system_3d_stats stats_;
    };

    void render_prepare_system_3d(system_context& context, void* user_data);
}
