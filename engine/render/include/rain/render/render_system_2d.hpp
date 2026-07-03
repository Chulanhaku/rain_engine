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

		void set_entity_query(const entity_query_desc& desc);

		[[nodiscard]] const entity_query_desc& entity_query()const;

		[[nodiscard]] u32 last_quad_count()const;
	private:
		[[nodiscard]] static entity_query_desc make_default_entity_query();
	private:
		sprite_renderer_2d sprite_renderer_;
		
		entity_query_desc entity_query_;
		u32 last_quad_count_ = 0;
	};
}