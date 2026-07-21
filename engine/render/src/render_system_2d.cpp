#include <rain/render/render_system_2d.hpp>

#include<rain/render/sprite_2d_component.hpp>
#include<rain/runtime/transform_2d_component.hpp>

namespace rain {
	render_system_2d::render_system_2d(
    render_backend& backend,
    material_2d_registry& materials,
    u32 max_quads)
    : sprite_renderer_(backend, max_quads)
    , materials_(&materials)
    , entity_query_(make_default_entity_query())
	{
	}

	void render_system_2d::render(world& target_world, const camera_2d& camera) {
		const entity_query_result entities = target_world.query_entities(entity_query_);

		if (entities.empty()) {
			last_quad_count_ = 0;
			return;
		}

		//auto* sprite_pool = target_world.try_get_component_pool<sprite_2d_component>();

		//auto* transform_pool = target_world.try_get_component_pool<transform_2d_component>();

		//if (sprite_pool == nullptr || transform_pool == nullptr) {
		//	last_quad_count_ = 0;
		//	return;
		//}

		//const auto& entities = sprite_pool->entities();
		//const auto& sprites = sprite_pool->values();

		sprite_renderer_.begin(camera);

		for (auto entity:entities) {

			if (!target_world.is_entity_active(entity))continue;

			const sprite_2d_component& sprite = target_world.get_component<sprite_2d_component>(entity);
			

			const material_2d* material = materials_->try_get(sprite.material);

			if (material == nullptr) {
				material = materials_->try_get(materials_->default_material());
			}

			if (material == nullptr)continue;

			const transform_2d_component& transform = target_world.get_component<transform_2d_component>(entity);

			sprite_renderer_.draw_rect_world(
				sprite_rect_world{
					.center = vec2{
						.x = transform.position.x,
						.y = transform.position.y
					},
					.size = vec2{
						.x = sprite.size.x * transform.scale.x,
						.y = sprite.size.y * transform.scale.y
					}
				}, 
				sprite.tint,
				material->texture,
				sprite.uv,
				material->blend_mode
			);

		}

		last_quad_count_ = sprite_renderer_.quad_count();

		sprite_renderer_.end();
	}

	u32 render_system_2d::last_quad_count()const {
		return last_quad_count_;
	}

	void render_system_2d::set_entity_query(const entity_query_desc&desc) {
		entity_query_ = desc;
	}

	const entity_query_desc& render_system_2d::entity_query()const {
		return entity_query_;
	}

	entity_query_desc render_system_2d::make_default_entity_query() {
		tag_query render_query;
		render_query.require_all(tag_id{ "object.renderable" });
		render_query.reject(tag_id{ "render.hidden" });

		return entity_query_desc{
			.required_components = {get_type_id<transform_2d_component>(),get_type_id<sprite_2d_component>()},
			.required_tags = render_query,
			.require_alive = true,
			.require_active = true
		};
	}
}
