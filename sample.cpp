#include <rain/core/event/event_debug_dump.hpp>
#include <rain/core/event/event_graph_export.hpp>
#include <rain/core/event/event_system.hpp>
#include <rain/core/event/event_trace_scope.hpp>
#include <rain/core/log.hpp>
#include <rain/runtime/system_scheduler.hpp>
#include <rain/runtime/transform_2d_component.hpp>
#include <rain/runtime/velocity_2d_component.hpp>
#include <rain/runtime/world.hpp>

#include <cstdio>
#include <fstream>

struct entity_moved_event
{
    rain::entity_id entity;
    rain::f32 x = 0.0f;
    rain::f32 y = 0.0f;
};

static void on_entity_moved_log(
    const entity_moved_event& event,
    const rain::event_context& context)
{
    std::printf(
        "[listener log] event=%s, source=%s, entity=%u, pos=(%.2f, %.2f), reason=%s\n",
        context.event_name.c_str(),
        context.source_name.c_str(),
        event.entity.index,
        event.x,
        event.y,
        context.reason.c_str()
    );
}

static void on_entity_moved_debug(
    const entity_moved_event& event,
    const rain::event_context& context)
{
    std::printf(
        "[listener debug] seq=%llu, frame=%llu, line=%u, entity=%u, pos=(%.2f, %.2f)\n",
        static_cast<unsigned long long>(context.sequence),
        static_cast<unsigned long long>(context.frame_index),
        context.line,
        event.entity.index,
        event.x,
        event.y
    );
}

static void movement_system(rain::system_context& context, void* user_data)
{
    (void)user_data;

    rain::world& target_world = *context.target_world;
    rain::event_system& events = *context.events;

    auto* velocity_pool =
        target_world.try_get_component_pool<rain::velocity_2d_component>();

    auto* transform_pool =
        target_world.try_get_component_pool<rain::transform_2d_component>();

    if (velocity_pool == nullptr || transform_pool == nullptr)
    {
        return;
    }

    const auto& entities = velocity_pool->entities();
    const auto& velocities = velocity_pool->values();

    for (rain::usize i = 0; i < velocity_pool->size(); ++i)
    {
        const rain::entity_id entity = entities[i];

        if (!transform_pool->has(entity))
        {
            continue;
        }

        rain::transform_2d_component& transform = transform_pool->get(entity);
        const rain::velocity_2d_component& velocity = velocities[i];

        transform.x += velocity.x * context.delta_seconds;
        transform.y += velocity.y * context.delta_seconds;

        events.enqueue<entity_moved_event>(
            entity_moved_event{
                .entity = entity,
                .x = transform.x,
                .y = transform.y
            },
            rain::event_emit_desc{
                .source_name = "system.movement",
                .reason = "velocity updated transform"
            }
        );
    }
}

int main()
{
    rain::log_info("Rain Engine 0.1 fourth batch event debug toolkit test started");

    rain::world game_world;
    rain::event_system events;
    rain::system_scheduler scheduler;

    events.register_event<entity_moved_event>({
        .event_name = "runtime.entity_moved",
        .category = "runtime",
        .allow_immediate_dispatch = true,
        .allow_queued_dispatch = true,
        .trace_enabled = true
    });

    events.add_listener<entity_moved_event>({
        .listener_name = "sample.debug_entity_moved",
        .owner_name = "sample_2d_action",
        .priority = 100,
        .enabled = true,
        .callback = &on_entity_moved_debug
    });

    events.add_listener<entity_moved_event>({
        .listener_name = "sample.log_entity_moved",
        .owner_name = "sample_2d_action",
        .priority = 0,
        .enabled = true,
        .callback = &on_entity_moved_log
    });

    scheduler.add_system({
        .system_name = "system.movement",
        .owner_name = "runtime",
        .phase_name = "update",
        .priority = 0,
        .enabled = true,
        .function = &movement_system,
        .user_data = nullptr
    });

    rain::entity_id player = game_world.create_entity();

    game_world.add_component<rain::transform_2d_component>(
        player,
        rain::transform_2d_component{
            .x = 10.0f,
            .y = 20.0f,
            .rotation = 0.0f,
            .scale_x = 1.0f,
            .scale_y = 1.0f
        }
    );

    game_world.add_component<rain::velocity_2d_component>(
        player,
        rain::velocity_2d_component{
            .x = 3.0f,
            .y = -2.0f
        }
    );

    {
        rain::scoped_event_trace trace_scope(events, true);

        scheduler.run(game_world, events, 0.5f, 1);
        events.dispatch_all_queued();
    }

    std::printf("\n%s", rain::dump_event_registry(events).c_str());
    std::printf("\n%s", rain::dump_event_listener_table(events).c_str());
    std::printf("\n%s", rain::dump_event_trace(events).c_str());

    rain::event_trace_filter movement_filter;
    movement_filter.source_name_contains = "system.movement";

    std::printf(
        "\n[filtered trace: source contains system.movement]\n%s",
        rain::dump_event_trace(events, movement_filter).c_str()
    );

    const std::string dot_text =
        rain::export_event_trace_dot(events);

    std::ofstream dot_file("event_trace.dot");
    dot_file << dot_text;
    dot_file.close();

    std::printf("\n[event graph]\n");
    std::printf("event_trace.dot exported\n");

    const rain::transform_2d_component& transform =
        game_world.get_component<rain::transform_2d_component>(player);

    std::printf(
        "\nfinal_position: %.2f, %.2f\n",
        transform.x,
        transform.y
    );

    rain::log_info("Rain Engine 0.1 fourth batch event debug toolkit test finished");

    return 0;
}