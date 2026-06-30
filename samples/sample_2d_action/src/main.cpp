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

#include "sample_2d_world_builder.hpp"

#include <rain/app/application.hpp>
#include <rain/app/layer.hpp>
#include <rain/platform/input_action.hpp>
#include <rain/platform/key_code.hpp>
#include <rain/render/camera_2d.hpp>
#include <rain/render/render_clear_color.hpp>
#include <rain/render/render_system_2d.hpp>
#include <rain/runtime/movement_system_2d.hpp>
#include <rain/runtime/transform_2d_component.hpp>
#include <rain/runtime/velocity_2d_component.hpp>

#include <memory>

static void sample_bounce_system(rain::system_context& context, void* user_data)
{
    (void)user_data;

    if (context.target_world == nullptr)
    {
        return;
    }

    rain::world& target_world = *context.target_world;

    auto* transform_pool =
        target_world.try_get_component_pool<rain::transform_2d_component>();

    auto* velocity_pool =
        target_world.try_get_component_pool<rain::velocity_2d_component>();

    if (transform_pool == nullptr || velocity_pool == nullptr)
    {
        return;
    }

    const auto& entities = velocity_pool->entities();

    for (rain::usize i = 0; i < velocity_pool->size(); ++i)
    {
        const rain::entity_id entity = entities[i];

        if (!target_world.is_entity_active(entity))
        {
            continue;
        }

        if (!transform_pool->has(entity))
        {
            continue;
        }

        rain::transform_2d_component& transform =
            transform_pool->get(entity);

        rain::velocity_2d_component& velocity =
            velocity_pool->get(entity);

        constexpr rain::f32 min_x = -300.0f;
        constexpr rain::f32 max_x = 300.0f;

        if (transform.position.x < min_x)
        {
            transform.position.x = min_x;
            velocity.x = -velocity.x;
        }

        if (transform.position.x > max_x)
        {
            transform.position.x = max_x;
            velocity.x = -velocity.x;
        }
    }
}

class sample_layer final : public rain::layer
{
public:
    void on_attach(rain::application_context& context) override
    {
        bind_input_actions(*context.input);

        render_system_ = std::make_unique<rain::render_system_2d>(
            *context.renderer,
            4096
        );

        camera_ = rain::camera_2d(rain::camera_2d_desc{
            .position = rain::vec2{.x = 0.0f, .y = 0.0f},
            .viewport_width = static_cast<rain::f32>(context.renderer->width()),
            .viewport_height = static_cast<rain::f32>(context.renderer->height()),
            .zoom = 1.0f
        });

        context.scheduler->add_system({
            .system_name = "system.movement_2d",
            .owner_name = "runtime",
            .phase_name = "update",
            .priority = 0,
            .enabled = true,
            .function = &rain::movement_system_2d,
            .user_data = nullptr
        });

        context.scheduler->add_system({
            .system_name = "sample.bounce",
            .owner_name = "sample_2d_action",
            .phase_name = "update",
            .priority = -10,
            .enabled = true,
            .function = &sample_bounce_system,
            .user_data = nullptr
        });

        world_handles_ = sample_2d::build_sample_2d_world(*context.target_world);
    }

    void on_update(rain::application_context& context) override
    {
        if (context.input->is_pressed(action_quit_))
        {
            context.main_window->request_close();
            return;
        }

        if (context.input->is_pressed(action_toggle_frozen_))
        {
            toggle_moving_entity_frozen(*context.target_world);
            return;
        }

        camera_.set_viewport_size(
            static_cast<rain::f32>(context.renderer->width()),
            static_cast<rain::f32>(context.renderer->height())
        );

        update_camera(context);
    }

    void on_render(rain::application_context& context) override
    {
        render_system_->render(*context.target_world, camera_);
    }

    void toggle_moving_entity_frozen(rain::world& target_world) {
        const rain::tag_id frozen_tag{"state.frozen"};

        if (target_world.has_tag(world_handles_.moving_rect, frozen_tag)) {
            target_world.remove_tag(world_handles_.moving_rect, frozen_tag);
        }
        else {
            target_world.add_tag(world_handles_.moving_rect, frozen_tag);
        }
    }

private:
    void bind_input_actions(rain::input_action_map& input)
    {
        input.bind_axis(action_camera_move_x_, rain::key_code::a, -1.0f);
        input.bind_axis(action_camera_move_x_, rain::key_code::d, 1.0f);

        input.bind_axis(action_camera_move_y_, rain::key_code::s, -1.0f);
        input.bind_axis(action_camera_move_y_, rain::key_code::w, 1.0f);

        input.bind_button(action_quit_, rain::key_code::escape);

        input.bind_button(action_toggle_frozen_, rain::key_code::space);
    }

    void update_camera(rain::application_context& context)
    {
        rain::vec2 camera_position = camera_.position();

        const rain::f32 camera_speed = 300.0f * context.delta_seconds;

        const rain::f32 move_x =
            context.input->get_axis(action_camera_move_x_);

        const rain::f32 move_y =
            context.input->get_axis(action_camera_move_y_);

        camera_position.x += move_x * camera_speed;
        camera_position.y += move_y * camera_speed;

        camera_.set_position(camera_position);
    }

private:
    rain::string_id action_camera_move_x_{"camera.move_x"};
    rain::string_id action_camera_move_y_{"camera.move_y"};
    rain::string_id action_quit_{"app.quit"};
    rain::string_id action_toggle_frozen_{"state.frozen"};

    std::unique_ptr<rain::render_system_2d> render_system_;

    rain::camera_2d camera_;
    sample_2d::sample_2d_world_handles world_handles_;
};

int main()
{
    rain::application app({
        .title = "Rain Engine 0.1 - World Builder V0",
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