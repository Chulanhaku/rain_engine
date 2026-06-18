#include <rain/render/render_system_2d.hpp>

#include<rain/render/sprite_2d_component.hpp>
#include<rain/runtime/transform_2d_component.hpp>

namespace rain {
	render_system_2d::render_system_2d(render_backend&backend ,u32 max_quads):sprite_renderer_(backend,max_quads){}

	void render_system_2d::render(world& target_world, const camera_2d& camera) {
		auto* sprite_pool = target_world.try_get_component_pool<sprite_2d_component>();

		auto* transform_pool = target_world.try_get_component_pool<transform_2d_component>();

		if (sprite_pool == nullptr || transform_pool == nullptr) {
			last_quad_count_ = 0;
			return;
		}

		const auto& entities = sprite_pool->entities();
		const auto& sprites = sprite_pool->values();

		sprite_renderer_.begin(camera);

		for (usize i = 0; i < sprite_pool->size(); i++) {
			const entity_id entity = entities[i];

			if (!target_world.is_entity_active(entity))continue;

			const sprite_2d_component& sprite = sprites[i];

			if (!sprite.visible)continue;

			if (!transform_pool->has(entity))continue;

			const transform_2d_component& transform = transform_pool->get(entity);

			sprite_renderer_.draw_rect_world(sprite_rect_world{
				.center = vec2{
					.x = transform.position.x,
					.y = transform.position.y
				},
				.size = vec2{
					.x = sprite.size.x * transform.scale.x,
					.y = sprite.size.y * transform.scale.y
				}
			}, sprite.color);

		}

		last_quad_count_ = sprite_renderer_.quad_count();

		sprite_renderer_.end();
	}

	u32 render_system_2d::last_quad_count()const {
		return last_quad_count_;
	}
}
