#include<rain/platform/window.hpp>

#include<rain/core/assert.hpp>
#include<rain/core/log.hpp>
#include <sstream>
#include<GLFW/glfw3.h>

namespace rain{
    namespace{
        u32 glfw_context_ref_count = 0;

        void glfw_error_callback(int error_code,const char* description){
            std::ostringstream stream;
            stream << "glfw error " << error_code << ": " << description;

            rain::log_error(stream.str());
        }

        void initialize_glfw(){
            if(glfw_context_ref_count>0){
                ++glfw_context_ref_count;
                return;
            }

            glfwSetErrorCallback(glfw_error_callback);

            const int init_result =glfwInit();

            rain_assert(init_result == GLFW_TRUE);

            ++glfw_context_ref_count;
        }

        void shutdown_glfw(){
            rain_assert(glfw_context_ref_count>0);

            --glfw_context_ref_count;

            if(glfw_context_ref_count==0){
                glfwTerminate();
            }

        }
    }

    rain_window::rain_window(const window_desc& desc) :width_(desc.width), height_(desc.height) {
        initialize_glfw();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);

        handle_ = glfwCreateWindow(static_cast<int>(desc.width), static_cast<int>(desc.height), desc.title.c_str(), nullptr, nullptr);

        rain_assert(handle_ != nullptr);

        glfwSetWindowUserPointer(handle_, this);
        glfwSetFramebufferSizeCallback(handle_, &rain_window::framebuffer_size_callback);

        rain::log_info("window created");

    }

    rain_window::~rain_window() {
        if (handle_ != nullptr) {
            glfwDestroyWindow(handle_);
            handle_ = nullptr;
        }
        shutdown_glfw();
    }

    void rain_window::poll_events() {
        glfwPollEvents();
    }

    void rain_window::present() {

    }

    bool rain_window::should_close()const {
        return glfwWindowShouldClose(handle_) == GLFW_TRUE;
    }

    void rain_window::request_close() {
        glfwSetWindowShouldClose(handle_, GLFW_TRUE);
    }

    bool rain_window::is_key_down(key_code key)const {
        const int state = glfwGetKey(handle_, static_cast<int>(key));
        return state == GLFW_TRUE || state == GLFW_REPEAT;
    }

    bool rain_window::is_mouse_button_down(mouse_button button)const {
        const int state = glfwGetMouseButton(handle_, static_cast<int>(button));
        return state == GLFW_PRESS;
    }

    f32 rain_window::mouse_x()const {
        double x = 0.0;
        double y = 0.0;
        glfwGetCursorPos(handle_, &x, &y);

        return static_cast<f32>(x);
    }

    f32 rain_window::mouse_y()const {
        double x = 0.0;
        double y = 0.0;
        glfwGetCursorPos(handle_, &x, &y);

        return static_cast<f32>(y);
    }

    u32 rain_window::width()const {
        return width_;
    }

    u32 rain_window::height()const {
        return height_;
    }

    void* rain_window::native_handle()const {
        return handle_;
    }

    void rain_window::framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height) {
        auto* target_window = static_cast<rain_window*>(glfwGetWindowUserPointer(glfw_window));

        if (target_window == nullptr)return;

        target_window->width_ = static_cast<u32>(width);
        target_window->height_ = static_cast<u32>(height);
    }
}