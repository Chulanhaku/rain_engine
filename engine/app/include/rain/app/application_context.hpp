#pragma once

#include <rain/core/event/event_system.hpp>
#include<rain/core/types.hpp>
#include<rain/platform/window.hpp>
#include<rain/runtime/system_scheduler.hpp>
#include<rain/runtime/world.hpp>
#include<rain/render/render_backend.hpp>
#include<rain/platform/input_action.hpp>
#include<rain/asset/texture_asset_registry.hpp>
#include<rain/render/material_2d_registry.hpp>
#include <rain/render/mesh_3d_registry.hpp>
#include <rain/render/material_3d_registry.hpp>

namespace rain {
	struct application_context {
		rain_window* main_window = nullptr;
		world* target_world = nullptr;
		event_system* events = nullptr;
		system_scheduler* scheduler = nullptr;
		render_backend* renderer = nullptr;
		texture_asset_registry* assets = nullptr;
		material_2d_registry* materials = nullptr;
		mesh_3d_registry* meshes_3d = nullptr;
		material_3d_registry* materials_3d = nullptr;

		f32 delta_seconds = 0.0f;
		u64 frame_index = 0;

		input_action_map* input = nullptr;
	};
}