#include "sample_3d_world_builder.hpp"

#include <rain/core/tag/tag.hpp>
#include <rain/render/camera_3d.hpp>
#include <rain/render/directional_light_3d_component.hpp>
#include <rain/render/mesh_3d_component.hpp>
#include <rain/runtime/angular_velocity_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>

namespace sample_3d
{
    sample_3d_world_handles build_sample_3d_world(
        rain::world& target_world,
        const sample_3d_world_resources& resources)
    {
        sample_3d_world_handles result{};

        result.camera =
            target_world.create_entity(
                rain::world_entity_desc{
                    .name = rain::string_id{
                        "entity.camera_3d"
                    },
                    .active = true
                }
            );

        target_world.add_component<
            rain::transform_3d_component
        >(
            result.camera,
            rain::transform_3d_component{
                .position = {
                    0.0f,
                    1.5f,
                    -6.0f
                },
                .rotation = {
                    -0.10f,
                    0.0f,
                    0.0f
                }
            }
        );

        target_world.add_component<
            rain::camera_3d_component
        >(
            result.camera,
            rain::camera_3d_component{}
        );

        target_world.add_tag(
            result.camera,
            rain::tag_id{"transform.3d"}
        );

        target_world.add_tag(
            result.camera,
            rain::tag_id{"camera.3d"}
        );

        target_world.add_tag(
            result.camera,
            rain::tag_id{"camera.active"}
        );

        result.cube =
            target_world.create_entity(
                rain::world_entity_desc{
                    .name = rain::string_id{
                        "entity.cube"
                    },
                    .active = true
                }
            );

        target_world.add_component<
            rain::transform_3d_component
        >(
            result.cube,
            rain::transform_3d_component{
                .position = {
                    0.0f,
                    0.0f,
                    0.0f
                },
                .rotation = {},
                .scale = {
                    1.0f,
                    1.0f,
                    1.0f
                }
            }
        );

        target_world.add_component<
            rain::mesh_3d_component
        >(
            result.cube,
            rain::mesh_3d_component{
                .mesh = resources.cube_mesh,
                .material = resources.cube_material,
                .tint = {
                    1.0f,
                    1.0f,
                    1.0f,
                    1.0f
                },
                .layer = 0
            }
        );

        target_world.add_component<
            rain::angular_velocity_3d_component
        >(
            result.cube,
            rain::angular_velocity_3d_component{
                .radians_per_second = {
                    0.25f,
                    0.7f,
                    0.1f
                }
            }
        );

        target_world.add_tag(
            result.cube,
            rain::tag_id{"transform.3d"}
        );

        target_world.add_tag(
            result.cube,
            rain::tag_id{"object.renderable"}
        );

        target_world.add_tag(
            result.cube,
            rain::tag_id{"render.3d"}
        );

        target_world.add_tag(
            result.cube,
            rain::tag_id{"object.rotatable"}
        );

        target_world.add_tag(result.cube, rain::tag_id{"render.frustum_cull"});

        result.directional_light =
            target_world.create_entity(
                rain::world_entity_desc{
                    .name = rain::string_id{
                        "entity.sun"
                    },
                    .active = true
                }
            );

        target_world.add_component<
            rain::directional_light_3d_component
        >(
            result.directional_light,
            rain::directional_light_3d_component{}
        );

        target_world.add_tag(
            result.directional_light,
            rain::tag_id{"light.directional"}
        );

        target_world.add_tag(
            result.directional_light,
            rain::tag_id{"light.active"}
        );

        return result;
    }
}