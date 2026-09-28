#include <rain/runtime/movement_system_3d.hpp>

#include <rain/runtime/local_matrix_3d_component.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/velocity_3d_component.hpp>
#include <rain/runtime/world.hpp>

#include <cmath>

namespace rain
{
    void movement_system_3d(system_context& context, void* user_data)
    {
        const auto target=user_data ? static_cast<const movement_settings_3d*>(user_data)->target :
            movement_target_3d::all;
        if (context.target_world == nullptr || context.entity_query == nullptr ||
            !std::isfinite(context.delta_seconds) || context.delta_seconds <= 0.0f)
        {
            return;
        }

        world& target_world = *context.target_world;
        const entity_query_result entities = target_world.query_entities(*context.entity_query);
        for (entity_id entity : entities)
        {
            const bool kinematic = target_world.has_tag_in_hierarchy(entity, tag_id{"physics.kinematic"});
            if ((target==movement_target_3d::kinematic && !kinematic) ||
                (target==movement_target_3d::non_kinematic && kinematic)) continue;
            if (target_world.has_tag_in_hierarchy(entity, tag_id{"state.frozen"}) ||
                target_world.has_tag_in_hierarchy(entity, tag_id{"physics.static"}) ||
                (!kinematic && target_world.has_tag_in_hierarchy(entity, tag_id{"physics.dynamic"})) ||
                target_world.has_component<local_matrix_3d_component>(entity))
            {
                continue;
            }

            auto* transform = target_world.try_get_component<transform_3d_component>(entity);
            const auto* velocity = target_world.try_get_component<velocity_3d_component>(entity);
            if (transform != nullptr && velocity != nullptr)
            {
                transform->position += velocity->linear * context.delta_seconds;
            }
        }
    }
}