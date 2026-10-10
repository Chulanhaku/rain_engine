#include <rain/runtime/physics_system_3d.hpp>
#include <rain/runtime/physics_world_3d.hpp>

namespace rain {
void physics_query_sync_system_3d(system_context& context,void* user_data) {
    if (context.target_world && user_data)
        static_cast<physics_world_3d*>(user_data)->sync_queries(*context.target_world);
}
void physics_world_system_3d(system_context& context,void* user_data) {
    if (!context.target_world || !user_data) return;
    auto& physics = *static_cast<physics_world_3d*>(user_data);
    physics.step(*context.target_world,context.delta_seconds,context.fixed_tick_index);
}
}
