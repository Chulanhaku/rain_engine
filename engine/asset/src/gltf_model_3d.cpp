#include <rain/asset/gltf_model_3d.hpp>

#include <rain/core/log.hpp>
#include <rain/core/math/mat4.hpp>
#include <rain/core/tag/tag.hpp>
#include <rain/render/mesh_3d_component.hpp>
#include <rain/runtime/local_matrix_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/world_transform_3d_component.hpp>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace rain {
    namespace {
        [[nodiscard]] mat4 convert_gltf_matrix_to_lh(
            const cgltf_float input[16]
        ) {
            constexpr f32 signs[4] = {
                1.0f,
                1.0f,
                -1.0f,
                1.0f
            };

            mat4 result{};
            for (u32 row = 0; row < 4; ++row) {
                for (u32 column = 0; column < 4; ++column) {
                    result.values[row][column] =
                        signs[row] *
                        input[row * 4 + column] *
                        signs[column];
                }
            }

            return result;
        }

        [[nodiscard]] std::string entity_name_for_node(
            const cgltf_node* node,
            usize node_index
        ) {
            if (node->name != nullptr && node->name[0] != '\0') {
                return node->name;
            }

            return "gltf.node." + std::to_string(node_index);
        }

        [[nodiscard]] const cgltf_accessor* find_attribute(
            const cgltf_primitive& primitive,
            cgltf_attribute_type type,
            cgltf_int index
        ) {
            for (cgltf_size attribute_index = 0;
                 attribute_index < primitive.attributes_count;
                 ++attribute_index) {
                const cgltf_attribute& attribute =
                    primitive.attributes[attribute_index];

                if (attribute.type == type && attribute.index == index) {
                    return attribute.data;
                }
            }

            return nullptr;
        }

        void generate_missing_normals(
            std::vector<mesh_vertex_3d>& vertices,
            const std::vector<u32>& indices
        ) {
            for (mesh_vertex_3d& vertex : vertices) {
                vertex.normal = {};
            }

            for (usize index = 0; index + 2 < indices.size(); index += 3) {
                const u32 index0 = indices[index];
                const u32 index1 = indices[index + 1];
                const u32 index2 = indices[index + 2];

                if (index0 >= vertices.size() ||
                    index1 >= vertices.size() ||
                    index2 >= vertices.size()) {
                    continue;
                }

                const vec3 edge1 =
                    vertices[index1].position - vertices[index0].position;
                const vec3 edge2 =
                    vertices[index2].position - vertices[index0].position;
                const vec3 face_normal = cross(edge1, edge2);

                for (u32 vertex_index : {index0, index1, index2}) {
                    mesh_vertex_3d& vertex = vertices[vertex_index];
                    vertex.normal = vertex.normal + face_normal;
                }
            }

            for (mesh_vertex_3d& vertex : vertices) {
                vertex.normal = normalize(vertex.normal);
                if (length_squared(vertex.normal) <= 0.000001f) {
                    vertex.normal = {0.0f, 1.0f, 0.0f};
                }
            }
        }

        [[nodiscard]] mesh_3d_handle import_primitive_mesh(
            const cgltf_primitive& primitive,
            const std::string& name,
            mesh_3d_registry& meshes
        ) {
            if (primitive.type != cgltf_primitive_type_triangles) {
                return {};
            }

            const cgltf_accessor* positions = find_attribute(
                primitive,
                cgltf_attribute_type_position,
                0
            );
            if (positions == nullptr || positions->count == 0) {
                return {};
            }

            const cgltf_accessor* normals = find_attribute(
                primitive,
                cgltf_attribute_type_normal,
                0
            );
            const cgltf_accessor* texcoords = find_attribute(
                primitive,
                cgltf_attribute_type_texcoord,
                0
            );

            std::vector<mesh_vertex_3d> vertices(positions->count);
            for (cgltf_size vertex_index = 0;
                 vertex_index < positions->count;
                 ++vertex_index) {
                cgltf_float position[3]{};
                if (!cgltf_accessor_read_float(
                        positions,
                        vertex_index,
                        position,
                        3)) {
                    return {};
                }

                mesh_vertex_3d vertex{};
                vertex.position = {
                    position[0],
                    position[1],
                    -position[2]
                };

                if (normals != nullptr && vertex_index < normals->count) {
                    cgltf_float normal[3]{};
                    if (cgltf_accessor_read_float(
                            normals,
                            vertex_index,
                            normal,
                            3)) {
                        vertex.normal = normalize(vec3{
                            normal[0],
                            normal[1],
                            -normal[2]
                        });
                    }
                }

                if (texcoords != nullptr && vertex_index < texcoords->count) {
                    cgltf_float uv[2]{};
                    if (cgltf_accessor_read_float(
                            texcoords,
                            vertex_index,
                            uv,
                            2)) {
                        vertex.uv = {uv[0], uv[1]};
                    }
                }

                vertices[vertex_index] = vertex;
            }

            std::vector<u32> indices;
            if (primitive.indices != nullptr) {
                indices.resize(primitive.indices->count);
                for (cgltf_size index = 0;
                     index < primitive.indices->count;
                     ++index) {
                    indices[index] = static_cast<u32>(
                        cgltf_accessor_read_index(primitive.indices, index)
                    );
                }
            }
            else {
                indices.resize(vertices.size());
                for (usize index = 0; index < vertices.size(); ++index) {
                    indices[index] = static_cast<u32>(index);
                }
            }

            for (usize index = 0; index + 2 < indices.size(); index += 3) {
                std::swap(indices[index + 1], indices[index + 2]);
            }

            if (normals == nullptr) {
                generate_missing_normals(vertices, indices);
            }

            return meshes.create(mesh_3d_desc{
                .name = name,
                .vertices = vertices,
                .indices = indices
            });
        }

        [[nodiscard]] texture_2d_handle import_material_texture(
            const cgltf_texture_view& view, const std::filesystem::path& directory,
            texture_asset_registry& textures, texture_format format) {
            if (view.texture == nullptr) return {};
            if (view.texcoord != 0 || view.has_transform) {
                log_warning("glTF material texture requires unsupported UV set/transform; using factors");
                return {};
            }
            const cgltf_image* image = view.texture->image;
            if (image == nullptr || image->uri == nullptr ||
                std::string{image->uri}.starts_with("data:")) {
                log_warning("glTF embedded material image is unsupported; using factors");
                return {};
            }
            // glTF external image URIs may contain percent-escaped path characters.
            std::string uri = image->uri;
            uri.resize(cgltf_decode_uri(uri.data()));
            const std::string path = (directory / uri).lexically_normal().generic_string();
            return textures.load_texture_2d(path.c_str(), format);
        }

        [[nodiscard]] material_3d_handle import_material(
            const cgltf_material* source_material,
            const std::filesystem::path& model_directory,
            material_3d_registry& materials, texture_asset_registry& textures) {
            material_3d_desc desc{};
            desc.name = source_material && source_material->name
                ? source_material->name : "gltf.material.default";
            // glTF's omitted metallic/roughness factors both default to one.
            desc.metallic_factor = 1.0f;
            desc.roughness_factor = 1.0f;
            desc.albedo_manual_srgb_decode = false;
            if (source_material == nullptr) return materials.create(desc);

            if (source_material->has_pbr_metallic_roughness) {
                const auto& pbr = source_material->pbr_metallic_roughness;
                desc.base_color = {pbr.base_color_factor[0], pbr.base_color_factor[1],
                                   pbr.base_color_factor[2], pbr.base_color_factor[3]};
                desc.metallic_factor = pbr.metallic_factor;
                desc.roughness_factor = pbr.roughness_factor;
                desc.albedo_texture = import_material_texture(pbr.base_color_texture,
                    model_directory, textures, texture_format::rgba8_unorm_srgb);
                desc.metallic_roughness_texture = import_material_texture(pbr.metallic_roughness_texture,
                    model_directory, textures, texture_format::rgba8_unorm);
            }
            desc.double_sided = source_material->double_sided != 0;
            if (source_material->alpha_mode == cgltf_alpha_mode_blend)
                desc.blend_mode = render_blend_mode::alpha;
            else if (source_material->alpha_mode == cgltf_alpha_mode_mask)
                desc.alpha_cutoff = source_material->alpha_cutoff;
            return materials.create(desc);
        }

        struct gltf_import_context {
            world* target_world = nullptr;
            mesh_3d_registry* meshes = nullptr;
            material_3d_registry* materials = nullptr;
            texture_asset_registry* textures = nullptr;
            const cgltf_data* data = nullptr;
            std::filesystem::path model_directory;
            gltf_model_3d_instance* instance = nullptr;
            std::vector<std::pair<
                const cgltf_material*,
                material_3d_handle
            >> material_cache;
        };

        [[nodiscard]] material_3d_handle resolve_material(
            gltf_import_context& context,
            const cgltf_material* material
        ) {
            for (const auto& cached : context.material_cache) {
                if (cached.first == material) {
                    return cached.second;
                }
            }

            const material_3d_handle result = import_material(
                material,
                context.model_directory,
                *context.materials,
                *context.textures
            );

            if (result != context.materials->default_material()) {
                context.material_cache.emplace_back(material, result);
                context.instance->owned_materials.push_back(result);
            }

            return result;
        }

        entity_id instantiate_node(
            gltf_import_context& context,
            const cgltf_node* node,
            entity_id parent
        ) {
            const usize node_index = static_cast<usize>(
                node - context.data->nodes
            );
            const std::string node_name =
                entity_name_for_node(node, node_index);

            const entity_id node_entity =
                context.target_world->create_entity(world_entity_desc{
                    .name = string_id{node_name.c_str()},
                    .active = true
                });
            context.instance->entities.push_back(node_entity);

            cgltf_float local_matrix[16]{};
            cgltf_node_transform_local(node, local_matrix);

            context.target_world->add_component<transform_3d_component>(
                node_entity,
                transform_3d_component{}
            );
            context.target_world->add_component<local_matrix_3d_component>(
                node_entity,
                local_matrix_3d_component{
                    .matrix = convert_gltf_matrix_to_lh(local_matrix)
                }
            );
            context.target_world->add_component<world_transform_3d_component>(
                node_entity,
                world_transform_3d_component{}
            );
            context.target_world->add_tag(
                node_entity,
                tag_id{"transform.3d"}
            );

            if (parent.is_valid()) {
                context.target_world->set_parent(node_entity, parent);
                context.target_world->add_tag(
                    node_entity,
                    tag_id{"transform.inherit_parent"}
                );
            }

            if (node->mesh != nullptr) {
                for (cgltf_size primitive_index = 0;
                     primitive_index < node->mesh->primitives_count;
                     ++primitive_index) {
                    const cgltf_primitive& primitive =
                        node->mesh->primitives[primitive_index];
                    const std::string primitive_name =
                        node_name + ".primitive." +
                        std::to_string(primitive_index);

                    const mesh_3d_handle mesh = import_primitive_mesh(
                        primitive,
                        primitive_name,
                        *context.meshes
                    );
                    if (!mesh.is_valid()) {
                        continue;
                    }

                    const material_3d_handle material = resolve_material(
                        context,
                        primitive.material
                    );
                    const entity_id primitive_entity =
                        context.target_world->create_entity(world_entity_desc{
                            .name = string_id{primitive_name.c_str()},
                            .active = true
                        });

                    context.instance->entities.push_back(primitive_entity);
                    context.instance->owned_meshes.push_back(mesh);

                    context.target_world->add_component<transform_3d_component>(
                        primitive_entity,
                        transform_3d_component{}
                    );
                    context.target_world->add_component<world_transform_3d_component>(
                        primitive_entity,
                        world_transform_3d_component{}
                    );
                    context.target_world->add_component<mesh_3d_component>(
                        primitive_entity,
                        mesh_3d_component{
                            .mesh = mesh,
                            .material = material,
                            .tint = {1.0f, 1.0f, 1.0f, 1.0f},
                            .layer = 0
                        }
                    );

                    context.target_world->set_parent(
                        primitive_entity,
                        node_entity
                    );
                    context.target_world->add_tag(
                        primitive_entity,
                        tag_id{"transform.3d"}
                    );
                    context.target_world->add_tag(
                        primitive_entity,
                        tag_id{"transform.inherit_parent"}
                    );
                    context.target_world->add_tag(
                        primitive_entity,
                        tag_id{"object.renderable"}
                    );
                    context.target_world->add_tag(
                        primitive_entity,
                        tag_id{"render.3d"}
                    );

                    context.target_world->add_tag(primitive_entity, tag_id("render.frustum_cull"));
                }
            }

            for (cgltf_size child_index = 0;
                 child_index < node->children_count;
                 ++child_index) {
                instantiate_node(
                    context,
                    node->children[child_index],
                    node_entity
                );
            }

            return node_entity;
        }
    }

    gltf_model_3d_instance instantiate_gltf_model_3d(
        const char* path,
        world& target_world,
        mesh_3d_registry& meshes,
        material_3d_registry& materials,
        texture_asset_registry& textures
    ) {
        gltf_model_3d_instance result{};
        if (path == nullptr || path[0] == '\0') {
            return result;
        }

        cgltf_options options{};
        cgltf_data* data = nullptr;

        const cgltf_result parse_result =
            cgltf_parse_file(&options, path, &data);
        if (parse_result != cgltf_result_success) {
            log_error(std::string{"failed to parse glTF: "} + path);
            return result;
        }

        const cgltf_result buffer_result =
            cgltf_load_buffers(&options, data, path);
        if (buffer_result != cgltf_result_success) {
            log_error(std::string{"failed to load glTF buffers: "} + path);
            cgltf_free(data);
            return result;
        }

        const cgltf_result validation_result = cgltf_validate(data);
        if (validation_result != cgltf_result_success) {
            log_error(std::string{"invalid glTF asset: "} + path);
            cgltf_free(data);
            return result;
        }

        const std::filesystem::path model_path{path};
        const std::string root_name = model_path.stem().string();
        result.root = target_world.create_entity(world_entity_desc{
            .name = string_id{root_name.c_str()},
            .active = true
        });
        result.entities.push_back(result.root);

        target_world.add_component<transform_3d_component>(
            result.root,
            transform_3d_component{}
        );
        target_world.add_component<world_transform_3d_component>(
            result.root,
            world_transform_3d_component{}
        );
        target_world.add_tag(result.root, tag_id{"transform.3d"});

        gltf_import_context context{
            .target_world = &target_world,
            .meshes = &meshes,
            .materials = &materials,
            .textures = &textures,
            .data = data,
            .model_directory = model_path.parent_path(),
            .instance = &result,
            .material_cache = {}
        };

        const cgltf_scene* scene = data->scene != nullptr
            ? data->scene
            : (data->scenes_count > 0 ? &data->scenes[0] : nullptr);

        if (scene != nullptr) {
            for (cgltf_size node_index = 0;
                 node_index < scene->nodes_count;
                 ++node_index) {
                instantiate_node(
                    context,
                    scene->nodes[node_index],
                    result.root
                );
            }
        }

        cgltf_free(data);

        log_info(
            std::string{"glTF instantiated: "} + path +
            " entities=" + std::to_string(result.entities.size()) +
            " meshes=" + std::to_string(result.owned_meshes.size()) +
            " materials=" + std::to_string(result.owned_materials.size())
        );

        return result;
    }

    void destroy_gltf_model_3d(
        gltf_model_3d_instance& instance,
        world& target_world,
        mesh_3d_registry& meshes,
        material_3d_registry& materials
    ) {
        for (auto iterator = instance.entities.rbegin();
             iterator != instance.entities.rend();
             ++iterator) {
            if (target_world.is_alive(*iterator)) {
                target_world.destroy_entity(*iterator);
            }
        }

        for (material_3d_handle material : instance.owned_materials) {
            materials.destroy(material);
        }
        for (mesh_3d_handle mesh : instance.owned_meshes) {
            meshes.destroy(mesh);
        }

        instance = {};
    }
}