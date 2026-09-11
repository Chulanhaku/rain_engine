// #include <rain/app/application.hpp>
// #include <rain/app/layer.hpp>
#include <rain/asset/gltf_model_3d.hpp>
#include <rain/core/log.hpp>
// #include <rain/core/event/event_debug_dump.hpp>
// #include <rain/core/log.hpp>
// #include <rain/platform/key_code.hpp>
// #include <rain/runtime/transform_2d_component.hpp>
// #include <rain/runtime/velocity_2d_component.hpp>
// #include<rain/render/d3d11/d3d11_render_backend.hpp>

// #include <cstdio>
// #include <cstdio>
#include <memory>

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


// gedaun


#include "sample_2d_world_builder.hpp"
#include "sample_3d_world_builder.hpp"

#include <rain/app/application.hpp>
#include <rain/app/layer.hpp>
#include <rain/asset/gltf_model_3d.hpp>
#include <rain/core/log.hpp>
#include <rain/platform/input_action.hpp>
#include <rain/platform/key_code.hpp>
#include <rain/render/camera_2d.hpp>
#include <rain/render/render_clear_color.hpp>
#include <rain/render/render_system_2d.hpp>
#include <rain/render/render_system_3d.hpp>
#include <rain/runtime/angular_velocity_3d_component.hpp>
#include <rain/runtime/rotation_system_3d.hpp>
#include <rain/runtime/transform_3d_component.hpp>
#include <rain/runtime/transform_hierarchy_system_3d.hpp>
#include <rain/render/sprite_2d_component.hpp>
#include <rain/runtime/movement_system_2d.hpp>
#include <rain/runtime/transform_2d_component.hpp>
#include <rain/runtime/velocity_2d_component.hpp>
#include<rain/asset/texture_asset_registry.hpp>

#include <cstdio>
#include <memory>

static void sample_bounce_system(rain::system_context& context, void* user_data)
{
    (void)user_data;

    if (context.target_world == nullptr || context.entity_query == nullptr)
    {
        return;
    }

    rain::world& target_world = *context.target_world;

    const rain::entity_query_result entities =
        target_world.query_entities(*context.entity_query);

    for (rain::entity_id entity : entities)
    {
        rain::transform_2d_component& transform =
            target_world.get_component<rain::transform_2d_component>(entity);

        rain::velocity_2d_component& velocity =
            target_world.get_component<rain::velocity_2d_component>(entity);

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
    void on_detach(rain::application_context& context) override {
        render_system_3d_.reset();
        render_system_.reset();

        if (context.materials == nullptr) {
            return;
        }

        context.materials->destroy(image_material_);

        context.materials->destroy(solid_material_);

        image_material_ = rain::material_2d_handle{};
        solid_material_ = rain::material_2d_handle{};

        if (context.materials_3d != nullptr) {
            context.materials_3d->destroy(cube_material_);
            cube_material_ = {};
        }
        if (context.meshes_3d != nullptr) {
            context.meshes_3d->destroy(cube_mesh_);
            cube_mesh_ = {};
        }

        rain::destroy_gltf_model_3d(model_, *context.target_world, *context.meshes_3d, *context.materials_3d);
    }

    void on_attach(rain::application_context& context) override
    {
        bind_input_actions(*context.input);

        camera_ = rain::camera_2d(rain::camera_2d_desc{
            .position = rain::vec2{.x = 0.0f, .y = 0.0f},
            .viewport_width = static_cast<rain::f32>(context.renderer->width()),
            .viewport_height = static_cast<rain::f32>(context.renderer->height()),
            .zoom = 1.0f
        });

        render_system_3d_ = std::make_unique<rain::render_system_3d>(
            *context.renderer, *context.meshes_3d, *context.materials_3d);

        context.scheduler->add_system({
            .system_name = "system.movement_2d",
            .owner_name = "runtime",
            .phase = rain::system_phase::movement,
            .priority = 0,
            .enabled = true,
            .entity_query = rain::entity_query_desc{
                .required_components = {
                    rain::get_type_id<rain::transform_2d_component>(),
                    rain::get_type_id<rain::velocity_2d_component>()
                },
                .required_tags = [] {
                    rain::tag_query query;
                    query.require_all(rain::tag_id{"object.movable"});
                    query.reject(rain::tag_id{"state.frozen"});
                    query.reject(rain::tag_id{"state.stunned"});
                    query.reject(rain::tag_id{"state.rooted"});
                    return query;
                }(),
                .require_alive = true,
                .require_active = true
            },
            .function = &rain::movement_system_2d,
            .user_data = nullptr
        });

        context.scheduler->add_system({
            .system_name = "sample.bounce",
            .owner_name = "sample_2d_action",
            .phase = rain::system_phase::movement,
            .priority = -10,
            .enabled = true,
            .entity_query = rain::entity_query_desc{
                .required_components = {
                    rain::get_type_id<rain::transform_2d_component>(),
                    rain::get_type_id<rain::velocity_2d_component>()
                },
                .required_tags = [] {
                    rain::tag_query query;
                    query.require_all(rain::tag_id{"object.movable"});
                    return query;
                }(),
                .require_alive = true,
                .require_active = true
            },
            .function = &sample_bounce_system,
            .user_data = nullptr
        });

        context.scheduler->add_system({
            .system_name = "system.rotation_3d",
            .owner_name = "runtime",
            .phase = rain::system_phase::movement,
            .priority = 0,
            .enabled = true,

            .entity_query = rain::entity_query_desc{
                .required_components = {
                    rain::get_type_id<
                        rain::transform_3d_component
                    >(),
                    rain::get_type_id<
                        rain::angular_velocity_3d_component
                    >()
                },

                .required_tags = [] {
                    rain::tag_query query;

                    query.require_all(
                        rain::tag_id{"object.rotatable"}
                    );

                    query.reject(
                        rain::tag_id{"state.frozen"}
                    );

                    return query;
                }(),

                .require_alive = true,
                .require_active = true
            },

                .function = &rain::rotation_system_3d,
                .user_data = nullptr
        });

        context.scheduler->add_system({
            .system_name = "system.render_prepare_3d",
            .owner_name = "render",
            .phase = rain::system_phase::render_prepare,
            .priority = 10,
            .enabled = true,
            .entity_query = {},
            .function = &rain::render_prepare_system_3d,
            .user_data = render_system_3d_.get()
        });

        context.scheduler->add_system({
            .system_name =
                "system.transform_hierarchy_3d",

            .owner_name = "runtime",

            .phase =
                rain::system_phase::post_update,

            .priority = 100,
            .enabled = true,

            .entity_query = rain::entity_query_desc{
                .required_components = {
                    rain::get_type_id<
                        rain::transform_3d_component
                    >()
                },

                .required_tags = [] {
                    rain::tag_query query;

                    query.require_all(
                        rain::tag_id{"transform.3d"}
                    );

                    query.reject(
                        rain::tag_id{"transform.disabled"}
                    );

                    return query;
                }(),

                .require_alive = true,
                .require_active = true
            },

            .function =
                &rain::transform_hierarchy_system_3d,

            .user_data = nullptr
        });

        for (const rain::system_debug_info& info : context.scheduler->debug_infos())
        {
            char message[256]{};
            std::snprintf(
                message,
                sizeof(message),
                "system registered: phase=%s priority=%d name=%s owner=%s components=%zu all_tags=%zu any_tags=%zu none_tags=%zu",
                rain::to_string(info.phase),
                info.priority,
                info.system_name.c_str(),
                info.owner_name.c_str(),
                info.required_component_count,
                info.required_all_tag_count,
                info.required_any_tag_count,
                info.rejected_tag_count
            );
            rain::log_info(message);
        }

        cube_mesh_ =
            context.meshes_3d->create_cube();

        const rain::texture_2d_handle texture =
            context.assets->load_texture_2d(
                "assets/textures/test.jpg"
            );

        cube_material_ =
            context.materials_3d->create(
                rain::material_3d_desc{
                    .name = "material.cube",
                    .albedo_texture = texture,
                    .base_color = {
                        1.0f,
                        1.0f,
                        1.0f,
                        1.0f
                    },
                    .blend_mode =
                        rain::render_blend_mode::opaque
                }
            );


        const rain::texture_2d_handle test_texture =
            context.assets->load_texture_2d(
                "assets/textures/test.jpg"
            );

        solid_material_ =
            context.materials->create(rain::material_2d_desc{
                .name = "material.solid_2d",
                .texture = rain::texture_2d_handle{},
                .blend_mode = rain::render_blend_mode::opaque
            });

        image_material_ =
            context.materials->create(rain::material_2d_desc{
                .name = "material.test_image",
                .texture = test_texture,
                .blend_mode = rain::render_blend_mode::alpha
            });

        render_system_ = std::make_unique<rain::render_system_2d>(
            *context.renderer,
            *context.materials,
            4096
        );

        model_ = rain::instantiate_gltf_model_3d("assets/models/test_model/model.gltf", *context.target_world, *context.meshes_3d, *context.materials_3d, *context.assets);
        rain::transform_3d_component* transform = context.target_world->try_get_component<rain::transform_3d_component>(model_.root);
        if (transform != nullptr) {
            transform->position = { 0.0f,0.0f,0.0f };

            transform->scale = {1.0f,1.0f,1.0f};
        }

        context.scheduler->add_system({
            .system_name = "system.render_prepare_2d",
            .owner_name = "render",
            .phase = rain::system_phase::render_prepare,
            .priority = 0,
            .enabled = true,

            .entity_query =
                rain::render_system_2d::make_entity_query(),

            .function = &rain::render_prepare_system_2d,
            .user_data = render_system_.get()
        });

        world_handles_ = sample_2d::build_sample_2d_world(
            *context.target_world,
            sample_2d::sample_2d_world_materials{
                .solid_material = solid_material_,
                .image_material = image_material_
            }
        );

        world_handles_3d_ = sample_3d::build_sample_3d_world(
            *context.target_world,
            sample_3d::sample_3d_world_resources{
                .cube_mesh = cube_mesh_,
                .cube_material = cube_material_
            }
        );
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

        if (context.input->is_pressed(action_toggle_hidden_))
        {
            toggle_green_rect_hidden(*context.target_world);
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
        (void)context;
        render_system_3d_->submit();
        render_system_->submit(camera_);
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

    void toggle_green_rect_hidden(rain::world&target_world) {
        const rain::tag_id hidden_tag{ "render.hidden" };

        if (target_world.has_tag(world_handles_.green_rect, hidden_tag)) {
            target_world.remove_tag(world_handles_.green_rect, hidden_tag);
        }
        else {
            target_world.add_tag(world_handles_.green_rect, hidden_tag);
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
        input.bind_button(action_toggle_hidden_, rain::key_code::h);
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
    rain::string_id action_toggle_hidden_{ "state.hidden" };


    std::unique_ptr<rain::render_system_2d> render_system_;
    std::unique_ptr<rain::render_system_3d> render_system_3d_;
    rain::material_2d_handle solid_material_;
    rain::material_2d_handle image_material_;

    rain::camera_2d camera_;
    sample_2d::sample_2d_world_handles world_handles_;
    sample_3d::sample_3d_world_handles world_handles_3d_;

    rain::mesh_3d_handle cube_mesh_;
    rain::material_3d_handle cube_material_;
    rain::gltf_model_3d_instance model_;
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

//ge1

// #include <iostream>
// #include <cstring>

// #ifdef _WIN32
// #include <winsock2.h>
// #include <ws2tcpip.h>
// #pragma comment(lib, "ws2_32.lib")
// using SOCKLEN = int;
// #else
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// using SOCKLEN = socklen_t;
// #define closesocket close
// #endif

// // 发包目标配置
// #define TARGET_IP   "162.14.132.34"  // 改成你的测试服务IP
// #define TARGET_PORT 40054          // 目标端口
// #define PACKET_DATA 0x56          // 单字节包内容 D
// #define SEND_COUNT  100000        // 发送总次数

// int main()
// {
// #ifdef _WIN32
//     // Windows 初始化网络库
//     WSADATA wsaData;
//     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
//     {
//         std::cerr << "WSAStartup failed" << std::endl;
//         return -1;
//     }
// #endif

//     // 1. 创建UDP socket
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0)
//     {
//         std::cerr << "Create socket failed" << std::endl;
// #ifdef _WIN32
//         WSACleanup();
// #endif
//         return -1;
//     }

//     // 2. 填充目标地址
//     sockaddr_in targetAddr{};
//     targetAddr.sin_family = AF_INET;
//     targetAddr.sin_port = htons(TARGET_PORT);
//     inet_pton(AF_INET, TARGET_IP, &targetAddr.sin_addr);

//     // 数据包：仅1字节 0x44
//     char buf = PACKET_DATA;
//     SOCKLEN addrLen = sizeof(targetAddr);

//     std::cout << "Start send UDP 0x44 packet, total: " << SEND_COUNT << std::endl;
//     while(1){
//         // 3. 循环 sendto 发包
//         for (int i = 0; i < SEND_COUNT; ++i)
//         {
//             ssize_t ret = sendto(
//                 sock,
//                 &buf,
//                 1,
//                 0,
//                 (sockaddr*)&targetAddr,
//                 addrLen
//             );

//             if (ret < 0)
//             {
//                 std::cerr << "Send fail at idx: " << i << std::endl;
//                 break;
//             }

//             if (i % 10000 == 0)
//             {
//                 std::cout << "Sent " << i << " packets..." << std::endl;
//             }
//         }
//     }
//     std::cout << "Send task finished" << std::endl;

//     // 释放资源
//     closesocket(sock);
// #ifdef _WIN32
//     WSACleanup();
// #endif
//     return 0;
// }


//ge2

// #include <algorithm>
// #include <chrono>
// #include <cstddef>
// #include <cstdint>
// #include <exception>
// #include <iostream>
// #include <string>
// #include <thread>
// #include <vector>

// #include <winsock2.h>
// #include <ws2tcpip.h>
// #include <windows.h>

// namespace
// {
//     constexpr std::uint32_t max_packets_per_second = 500000;
//     constexpr std::size_t min_packet_size = 1;
//     constexpr std::size_t max_packet_size = 1400;

//     bool is_escape_pressed()
//     {
//         return (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
//     }

//     bool is_private_or_loopback_ipv4(const in_addr& address)
//     {
//         const std::uint32_t host_address = ntohl(address.s_addr);

//         const std::uint8_t first_octet =
//             static_cast<std::uint8_t>((host_address >> 24) & 0xff);

//         const std::uint8_t second_octet =
//             static_cast<std::uint8_t>((host_address >> 16) & 0xff);

//         // 127.0.0.0/8
//         if (first_octet == 127)
//         {
//             return true;
//         }

//         // 10.0.0.0/8
//         if (first_octet == 10)
//         {
//             return true;
//         }

//         // 172.16.0.0/12
//         if (first_octet == 172 &&
//             second_octet >= 16 &&
//             second_octet <= 31)
//         {
//             return true;
//         }

//         // 192.168.0.0/16
//         if (first_octet == 192 &&
//             second_octet == 168)
//         {
//             return true;
//         }

//         return false;
//     }

//     void print_usage()
//     {
//         std::cout
//             << "usage:\n"
//             << "  udp_sender.exe <ip> <port> <pps> <packet_size>\n\n"
//             << "example:\n"
//             << "  udp_sender.exe 127.0.0.1 9000 100 1200\n\n"
//             << "parameters:\n"
//             << "  ip          target IPv4 address\n"
//             << "  port        target UDP port\n"
//             << "  pps         packets per second, 1 - "
//             << max_packets_per_second
//             << "\n"
//             << "  packet_size UDP payload size, "
//             << min_packet_size
//             << " - "
//             << max_packet_size
//             << " bytes\n";
//     }
// }

// int main(int argc, char* argv[])
// {
//     if (argc != 5)
//     {
//         print_usage();
//         return 1;
//     }

//     const std::string target_ip = argv[1];

//     int target_port = 0;
//     std::uint32_t packets_per_second = 0;
//     std::size_t packet_size = 0;

//     try
//     {
//         target_port = std::stoi(argv[2]);

//         packets_per_second =
//             static_cast<std::uint32_t>(
//                 std::stoul(argv[3]));

//         packet_size =
//             static_cast<std::size_t>(
//                 std::stoull(argv[4]));
//     }
//     catch (const std::exception&)
//     {
//         std::cerr
//             << "invalid port, pps or packet_size\n";

//         return 1;
//     }

//     if (target_port <= 0 ||
//         target_port > 65535)
//     {
//         std::cerr
//             << "port must be between 1 and 65535\n";

//         return 1;
//     }

//     if (packets_per_second == 0 ||
//         packets_per_second > max_packets_per_second)
//     {
//         std::cerr
//             << "pps must be between 1 and "
//             << max_packets_per_second
//             << '\n';

//         return 1;
//     }

//     if (packet_size < min_packet_size ||
//         packet_size > max_packet_size)
//     {
//         std::cerr
//             << "packet_size must be between "
//             << min_packet_size
//             << " and "
//             << max_packet_size
//             << " bytes\n";

//         return 1;
//     }

//     WSADATA winsock_data{};

//     const int startup_result =
//         WSAStartup(
//             MAKEWORD(2, 2),
//             &winsock_data);

//     if (startup_result != 0)
//     {
//         std::cerr
//             << "WSAStartup failed: "
//             << startup_result
//             << '\n';

//         return 1;
//     }

//     const SOCKET socket_handle =
//         socket(
//             AF_INET,
//             SOCK_DGRAM,
//             IPPROTO_UDP);

//     if (socket_handle == INVALID_SOCKET)
//     {
//         std::cerr
//             << "socket failed: "
//             << WSAGetLastError()
//             << '\n';

//         WSACleanup();
//         return 1;
//     }

//     sockaddr_in target_address{};
//     target_address.sin_family = AF_INET;
//     target_address.sin_port =
//         htons(
//             static_cast<unsigned short>(
//                 target_port));

//     const int address_result =
//         inet_pton(
//             AF_INET,
//             target_ip.c_str(),
//             &target_address.sin_addr);

//     if (address_result != 1)
//     {
//         std::cerr
//             << "invalid IPv4 address\n";

//         closesocket(socket_handle);
//         WSACleanup();

//         return 1;
//     }

//     if (!is_private_or_loopback_ipv4(
//             target_address.sin_addr))
//     {
//         std::cerr
//             << "only loopback or private IPv4 "
//             << "addresses are allowed\n"
//             << "allowed ranges:\n"
//             << "  127.0.0.0/8\n"
//             << "  10.0.0.0/8\n"
//             << "  172.16.0.0/12\n"
//             << "  192.168.0.0/16\n";

//         closesocket(socket_handle);
//         WSACleanup();

//         return 1;
//     }

//     std::vector<char> packet_data(packet_size);

//     for (std::size_t byte_index = 0;
//          byte_index < packet_data.size();
//          ++byte_index)
//     {
//         packet_data[byte_index] =
//             static_cast<char>(
//                 byte_index & 0xff);
//     }

//     // 前 8 字节写入一个简单标识，方便服务端识别测试包。
//     constexpr char packet_magic[] =
//         {'R', 'A', 'I', 'N', 'U', 'D', 'P', '1'};

//     const std::size_t magic_size =
//         std::min(
//             packet_data.size(),
//             sizeof(packet_magic));

//     std::copy_n(
//         packet_magic,
//         magic_size,
//         packet_data.begin());

//     using clock_type =
//         std::chrono::steady_clock;

//     const auto packet_interval =
//         std::chrono::duration<double>(
//             1.0 /
//             static_cast<double>(
//                 packets_per_second));

//     auto next_send_time =
//         clock_type::now();

//     auto statistics_start_time =
//         clock_type::now();

//     std::uint64_t total_packet_count = 0;
//     std::uint64_t total_byte_count = 0;

//     std::uint64_t current_packet_count = 0;
//     std::uint64_t current_byte_count = 0;

//     std::cout
//         << "UDP sender started\n"
//         << "target: "
//         << target_ip
//         << ':'
//         << target_port
//         << '\n'
//         << "target pps: "
//         << packets_per_second
//         << '\n'
//         << "payload size: "
//         << packet_size
//         << " bytes\n"
//         << "estimated payload bandwidth: "
//         << (
//             static_cast<double>(
//                 packets_per_second) *
//             static_cast<double>(
//                 packet_size) *
//             8.0 /
//             1000000.0)
//         << " Mbit/s\n"
//         << "press ESC to stop\n\n";

//     while (!is_escape_pressed())
//     {
//         const int sent_bytes =
//             sendto(
//                 socket_handle,
//                 packet_data.data(),
//                 static_cast<int>(
//                     packet_data.size()),
//                 0,
//                 reinterpret_cast<
//                     const sockaddr*>(
//                     &target_address),
//                 sizeof(target_address));

//         if (sent_bytes == SOCKET_ERROR)
//         {
//             std::cerr
//                 << "\nsendto failed: "
//                 << WSAGetLastError()
//                 << '\n';

//             break;
//         }

//         ++total_packet_count;
//         ++current_packet_count;

//         total_byte_count +=
//             static_cast<std::uint64_t>(
//                 sent_bytes);

//         current_byte_count +=
//             static_cast<std::uint64_t>(
//                 sent_bytes);

//         const auto current_time =
//             clock_type::now();

//         const auto statistics_duration =
//             current_time -
//             statistics_start_time;

//         if (statistics_duration >=
//             std::chrono::seconds(1))
//         {
//             const double elapsed_seconds =
//                 std::chrono::duration<double>(
//                     statistics_duration)
//                     .count();

//             const double actual_pps =
//                 static_cast<double>(
//                     current_packet_count) /
//                 elapsed_seconds;

//             const double payload_mbps =
//                 static_cast<double>(
//                     current_byte_count) *
//                 8.0 /
//                 elapsed_seconds /
//                 1000000.0;

//             std::cout
//                 << "\ractual pps: "
//                 << static_cast<std::uint64_t>(
//                     actual_pps)
//                 << " | payload: "
//                 << payload_mbps
//                 << " Mbit/s"
//                 << " | total packets: "
//                 << total_packet_count
//                 << "          "
//                 << std::flush;

//             current_packet_count = 0;
//             current_byte_count = 0;
//             statistics_start_time =
//                 current_time;
//         }

//         next_send_time +=
//             std::chrono::duration_cast<
//                 clock_type::duration>(
//                 packet_interval);

//         std::this_thread::sleep_until(
//             next_send_time);

//         const auto after_sleep_time =
//             clock_type::now();

//         // 程序卡顿后不补发之前积压的数据包。
//         if (next_send_time <
//             after_sleep_time -
//                 std::chrono::seconds(1))
//         {
//             next_send_time =
//                 after_sleep_time;
//         }
//     }

//     std::cout
//         << "\n\nUDP sender stopped\n"
//         << "total packets: "
//         << total_packet_count
//         << '\n'
//         << "total payload bytes: "
//         << total_byte_count
//         << '\n'
//         << "total payload MiB: "
//         << (
//             static_cast<double>(
//                 total_byte_count) /
//             1024.0 /
//             1024.0)
//         << '\n';

//     closesocket(socket_handle);
//     WSACleanup();

//     return 0;
// }
