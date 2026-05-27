#pragma once

#include<rain/app/application_context.hpp>
#include<rain/app/layer.hpp>
#include<rain/core/event/event_system.hpp>
#include<rain/core/types.hpp>
#include<rain/platform/window.hpp>
#include<rain/runtime/world.hpp>
#include<rain/render/render_backend.hpp>
#include<rain/render/render_clear_color.hpp>
#include<rain/render/d3d11/d3d11_render_backend.hpp>

#include<memory>
#include<string>
#include<vector>

namespace rain {
	struct application_desc
	{
		std::string title = "rain_engine";
		u32 width = 1280;
		u32 height = 720;
		bool resizable = true;
		render_clear_color clear_color{};
	};

	class application {
	public:
		explicit application(const application_desc& desc);
		~application();
		application(const application&) = delete;
		application& operator= (const application&) = delete;
		application(const application&&) = delete;
		application& operator= (const application&&) = delete;

		void push_layer(std::unique_ptr<layer>new_layer);

		int run();

		void request_close();

		[[nodiscard]] rain_window& main_window();
		[[nodiscard]] const rain_window& main_window()const;

		[[nodiscard]] world& target_world();
		[[nodiscard]] const world& target_world() const;

		[[nodiscard]] event_system& events();
		[[nodiscard]] const event_system& events() const;

		[[nodiscard]] system_scheduler& scheduler();
		[[nodiscard]] const system_scheduler& scheduler() const;

		[[nodiscard]] render_backend& renderer();
		[[nodiscard]] const render_backend& renderer()const;

	private:
		[[nodiscard]] application_context make_context(f32 delta_seconds);

	private:
		rain_window main_window_;
		world target_world_;
		event_system events_;
		system_scheduler scheduler_;
		std::vector<std::unique_ptr<layer>> layers_;

		bool running_ = false;
		u64 frame_index_ = 0;

		std::unique_ptr<render_backend>renderer_;
		render_clear_color  clear_color_{};
	};
}