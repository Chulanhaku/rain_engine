#pragma once
#include <rain/runtime/system_scheduler.hpp>

namespace rain {
// Register in the fixed physics phase with a persistent physics_world_3d* in
// user_data. The world owns component queries, contact cache and event delivery.
void physics_world_system_3d(system_context& context,void* user_data);
// Optional variable-frame sync after edits/movement and before picking/queries.
// Same persistent physics_world_3d* user_data as the simulation system.
void physics_query_sync_system_3d(system_context& context,void* user_data);
}
