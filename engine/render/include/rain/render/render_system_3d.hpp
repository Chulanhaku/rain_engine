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
    struct camera_3d_frame {
        mat4 view;
        mat4 projection;
        mat4 view_projection;
        vec3 position;
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
        pipeline_state_handle pipeline_;
        render_buffer_handle object_constants_;
        render_buffer_handle scene_constants_;
        render_buffer_handle material_constants_;
        bool has_camera_ = false;
    };

    void render_prepare_system_3d(system_context& context, void* user_data);
}
