#pragma once

#include <rain/asset/texture_asset_registry.hpp>
#include <rain/render/material_3d_registry.hpp>
#include <rain/render/mesh_3d_registry.hpp>
#include <rain/runtime/entity.hpp>
#include <rain/runtime/world.hpp>

#include <vector>

namespace rain {
    struct gltf_model_3d_instance {
        entity_id root;
        std::vector<entity_id> entities;
        std::vector<mesh_3d_handle> owned_meshes;
        std::vector<material_3d_handle> owned_materials;

        [[nodiscard]] bool is_valid() const {
            return root.is_valid();
        }
    };

    [[nodiscard]] gltf_model_3d_instance instantiate_gltf_model_3d(
        const char* path,
        world& target_world,
        mesh_3d_registry& meshes,
        material_3d_registry& materials,
        texture_asset_registry& textures
    );

    void destroy_gltf_model_3d(
        gltf_model_3d_instance& instance,
        world& target_world,
        mesh_3d_registry& meshes,
        material_3d_registry& materials
    );
}