// #include <rain/app/application.hpp>
// #include <rain/app/layer.hpp>
// #include <rain/core/event/event_debug_dump.hpp>
// #include <rain/core/log.hpp>
// #include <rain/platform/key_code.hpp>
// #include <rain/runtime/transform_2d_component.hpp>
// #include <rain/runtime/velocity_2d_component.hpp>
// #include<rain/render/d3d11/d3d11_render_backend.hpp>

// #include <cstdio>
// #include <memory>

// struct entity_moved_event
// {
//     rain::entity_id entity;
//     rain::f32 x = 0.0f;
//     rain::f32 y = 0.0f;
// };

// static void on_entity_moved(
//     const entity_moved_event& event,
//     const rain::event_context& context)
// {
//     if (context.frame_index > 5)
//     {
//         return;
//     }

//     std::printf(
//         "[event] frame=%llu, source=%s, entity=%u, position=(%.2f, %.2f)\n",
//         static_cast<unsigned long long>(context.frame_index),
//         context.source_name.c_str(),
//         event.entity.index,
//         event.x,
//         event.y
//     );
// }

// static void movement_system(rain::system_context& context, void* user_data)
// {
//     (void)user_data;

//     rain::world& target_world = *context.target_world;
//     rain::event_system& events = *context.events;

//     auto* velocity_pool =
//         target_world.try_get_component_pool<rain::velocity_2d_component>();

//     auto* transform_pool =
//         target_world.try_get_component_pool<rain::transform_2d_component>();

//     if (velocity_pool == nullptr || transform_pool == nullptr)
//     {
//         return;
//     }

//     const auto& entities = velocity_pool->entities();
//     const auto& velocities = velocity_pool->values();

//     for (rain::usize i = 0; i < velocity_pool->size(); ++i)
//     {
//         const rain::entity_id entity = entities[i];

//         if (!transform_pool->has(entity))
//         {
//             continue;
//         }

//         rain::transform_2d_component& transform = transform_pool->get(entity);
//         const rain::velocity_2d_component& velocity = velocities[i];

//         transform.position.x += velocity.x * context.delta_seconds;
//         transform.position.y += velocity.y * context.delta_seconds;

//         events.enqueue<entity_moved_event>(
//             entity_moved_event{
//                 .entity = entity,
//                 .x = transform.position.x,
//                 .y = transform.position.y
//             },
//             rain::event_emit_desc{
//                 .source_name = "system.movement",
//                 .reason = "velocity updated transform"
//             }
//         );
//     }
// }

// class sample_game_layer final : public rain::layer
// {
// public:
//     void on_attach(rain::application_context& context) override
//     {
//         rain::log_info("sample_game_layer attached");

//         context.events->register_event<entity_moved_event>({
//             .event_name = "runtime.entity_moved",
//             .category = "runtime",
//             .allow_immediate_dispatch = true,
//             .allow_queued_dispatch = true,
//             .trace_enabled = true
//         });

//         context.events->add_listener<entity_moved_event>({
//             .listener_name = "sample.print_entity_moved",
//             .owner_name = "sample_2d_action",
//             .priority = 0,
//             .enabled = true,
//             .callback = &on_entity_moved
//         });

//         context.scheduler->add_system({
//             .system_name = "system.movement",
//             .owner_name = "runtime",
//             .phase_name = "update",
//             .priority = 0,
//             .enabled = true,
//             .function = &movement_system,
//             .user_data = nullptr
//         });

//         player_ = context.target_world->create_entity();

//         context.target_world->add_component<rain::transform_2d_component>(
//             player_,
//             rain::transform_2d_component{
//                 .position = rain::vec2{10.0f, 20.0f},
//                 .rotation = 0.0f,
//                 .scale = rain::vec2{1.0f, 1.0f}
//             }
//         );

//         context.target_world->add_component<rain::velocity_2d_component>(
//             player_,
//             rain::velocity_2d_component{
//                 .x = 20.0f,
//                 .y = 0.0f
//             }
//         );

//         std::printf("%s\n", rain::dump_event_registry(*context.events).c_str());
//     }

//     void on_update(rain::application_context& context) override
//     {
//         if (context.main_window->is_key_down(rain::key_code::escape))
//         {
//             context.main_window->request_close();
//             return;
//         }

//         if (context.frame_index == 60)
//         {
//             const rain::transform_2d_component& transform =
//                 context.target_world->get_component<rain::transform_2d_component>(player_);

//             std::printf(
//                 "[sample] frame=60, player_position=(%.2f, %.2f), mouse=(%.2f, %.2f)\n",
//                 transform.position.x,
//                 transform.position.y,
//                 context.main_window->mouse_x(),
//                 context.main_window->mouse_y()
//             );
//         }
//     }

//     void on_detach(rain::application_context& context) override
//     {
//         rain::log_info("sample_game_layer detached");

//         if (context.target_world->is_alive(player_))
//         {
//             const rain::transform_2d_component& transform =
//                 context.target_world->get_component<rain::transform_2d_component>(player_);

//             std::printf(
//                 "[sample] final player_position=(%.2f, %.2f)\n",
//                 transform.position.x,
//                 transform.position.y
//             );
//         }
//     }

//     void on_render(rain::application_context& context) override {
//         context.renderer->draw_debug_triangle();
//     }
// private:
//     rain::entity_id player_;
// };

// int main()
// {
//     rain::application app({
//         .title = "Rain Engine 0.1 - sample_2d_action",
//         .width = 1280,
//         .height = 720,
//         .resizable = true,
//         .clear_color = rain::render_clear_color{
//             .r =0.08f,
//             .g=0.10f,
//             .b= 0.16f,
//             .a = 1.0f,
// }
//     });

//     app.push_layer(std::make_unique<sample_game_layer>());

//     return app.run();
// }

#include <rain/app/application.hpp>
#include <rain/app/layer.hpp>
#include <rain/platform/key_code.hpp>
#include <rain/render/render_clear_color.hpp>
#include <rain/render/sprite_renderer_2d.hpp>

#include <cmath>
#include <memory>

class sample_layer final : public rain::layer
{
public:
    void on_attach(rain::application_context& context) override
    {
        sprite_renderer_ = std::make_unique<rain::sprite_renderer_2d>(
            *context.renderer,
            1024
        );
    }

    void on_update(rain::application_context& context) override
    {
        if (context.main_window->is_key_down(rain::key_code::escape))
        {
            context.main_window->request_close();
        }
    }

    void on_render(rain::application_context& context) override
    {
        (void)context;

        const float time = static_cast<float>(context.frame_index) * 0.016f;
        const float x = std::sin(time) * 0.45f;

        sprite_renderer_->begin();

        sprite_renderer_->draw_rect(
            rain::sprite_rect{
                .x = x - 0.15f,
                .y = 0.20f,
                .width = 0.30f,
                .height = 0.30f
            },
            rain::sprite_color{
                .r = 0.20f,
                .g = 0.75f,
                .b = 1.00f,
                .a = 1.00f
            }
        );

        sprite_renderer_->draw_rect(
            rain::sprite_rect{
                .x = -0.65f,
                .y = -0.20f,
                .width = 0.25f,
                .height = 0.25f
            },
            rain::sprite_color{
                .r = 1.00f,
                .g = 0.45f,
                .b = 0.25f,
                .a = 1.00f
            }
        );

        sprite_renderer_->draw_rect(
            rain::sprite_rect{
                .x = 0.40f,
                .y = -0.10f,
                .width = 0.35f,
                .height = 0.20f
            },
            rain::sprite_color{
                .r = 0.35f,
                .g = 1.00f,
                .b = 0.45f,
                .a = 1.00f
            }
        );

        sprite_renderer_->end();
    }

private:
    std::unique_ptr<rain::sprite_renderer_2d> sprite_renderer_;
};

int main()
{
    rain::application app({
        .title = "Rain Engine 0.1 - Sprite Renderer 2D",
        .width = 1280,
        .height = 720,
        .resizable = true,
        .clear_color = rain::render_clear_color{
            .r = 0.08f,
            .g = 0.10f,
            .b = 0.16f,
            .a = 1.0f
        }
    });

    app.push_layer(std::make_unique<sample_layer>());

    return app.run();
}