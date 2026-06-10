#pragma once

#include <rain/render/camera_2d.hpp>
#include<rain/render/sprite_renderer_2d.hpp>
#include<rain/runtime/world.hpp>

namespace rain {
	class render_system_2d {
	public:
		explicit render_system_2d(render_backend& backend, u32 max_quads = 4096);

		render_system_2d(const render_system_2d&) = delete;
		render_system_2d& operator =(const render_system_2d&) = delete;

		void render(world& target_world, const camera_2d & camera);

		[[nodiscard]] u32 last_quad_count()const;

	private:
		sprite_renderer_2d sprite_renderer_;
		u32 last_quad_count_ = 0;
	};
}