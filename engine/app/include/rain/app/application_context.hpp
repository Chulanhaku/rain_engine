#pragma once

#include <rain/core/event/event_system.hpp>
#include<rain/core/types.hpp>
#include<rain/platform/window.hpp>
#include<rain/runtime/system_scheduler.hpp>
#include<rain/runtime/world.hpp>

namespace rain {
	struct application_context {
		rain_window* main_window = nullptr;
		world* target_world = nullptr;
		event_system* events = nullptr;
		system_scheduler* scheduler = nullptr;

		f32 delta_seconds = 0.0f;
		u64 frame_index = 0;
	};
}