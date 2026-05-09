#pragma once

#include<rain/core/types.hpp>
#include<rain/platform/key_code.hpp>

#include<string>

struct GLFWwindow;

namespace rain{
    struct window_desc{
        std::string title = "RAIN Engine";
        u32 width = 1280;
        u32 height = 720;
        bool resizable = true;
    };

    class rain_window{
    public:
        explicit rain_window (const window_desc& desc);
        ~rain_window();

        rain_window(const rain_window&) = delete;
        rain_window& operator=(const rain_window&)=delete;
        rain_window(const rain_window&&) = delete;
        rain_window& operator=(const rain_window&&)=delete;

        void poll_events();

        void present();

        [[nodiscard]]bool should_close()const;

        void request_close();

        [[nodiscard]]bool is_key_down(key_code key)const;
        [[nodiscard]]bool is_mouse_button_down(mouse_button button)const;

        [[nodiscard]]f32 mouse_x()const;
        [[nodiscard]]f32 mouse_y()const;

        [[nodiscard]]u32 width()const;
        [[nodiscard]]u32 height()const;

        [[nodiscard]]void* native_handle()const;

    private:
        static void framebuffer_size_callback(GLFWwindow*glfw_window,int width,int height);

    private:
        GLFWwindow* handle_ = nullptr;
        u32 width_ = 0;
        u32 height_ = 0;
    };
}