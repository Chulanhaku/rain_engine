#include "sample_2d_world_builder.hpp"

#include<rain/core/string_id.hpp>
#include<rain/core/math/vec2.hpp>
#include<rain/render/sprite_2d_component.hpp>
#include<rain/runtime/transform_2d_component.hpp>
#include<rain/runtime/velocity_2d_component.hpp>


namespace sample_2d{
	namespace {
		rain::entity_id create_rect(
			rain::world& target_world,
			rain::string_id name,
			rain::f32 x,
			rain::f32 y,
			rain::f32 width,
			rain::f32 height,
			rain::material_2d_handle material,
			const rain::sprite_color &tint,
			rain::i32 order_in_layer
		)
		{
			rain::entity_id entity = target_world.create_entity(rain::world_entity_desc{
				.name = name,
				.active = true
			});
			target_world.add_tag(entity, rain::tag_id{ "object.renderable" });
			target_world.add_component<rain::transform_2d_component>(entity, rain::transform_2d_component{
					.position = rain::vec2{.x = x, .y = y},
					.rotation = 0.0f,
					.scale = {1.0f,1.0f}
				}
			);

			target_world.add_component < rain::sprite_2d_component > (entity, rain::sprite_2d_component{
					.size = rain::vec2{.x = width,.y = height},
					.tint = tint,
					.material = material,
					.uv = {},
					.layer = 0,
					.order_in_layer = order_in_layer
				}
			);

			return entity;
		
		}


	}

	sample_2d_world_handles build_sample_2d_world(rain::world& target_world, const sample_2d_world_materials& materials) {
		sample_2d_world_handles handles{};

		handles.moving_rect = create_rect(
			target_world,
			rain::string_id{ "entity.moving_rect" },
			0.0f,
			80.0f,
			120.0f,
			120.0f,
			materials.solid_material,
			rain::sprite_color{
				.r = 0.20f,
				.g = 0.75f,
				.b = 1.00f,
				.a = 1.00f
			},
			0
		);

		target_world.add_component<rain::velocity_2d_component>(
			handles.moving_rect,
			rain::velocity_2d_component{
				.x = 180.0f,
				.y = 0.0f
			}
		);

		target_world.add_tag(handles.moving_rect, rain::tag_id{ "object.movable" });
		target_world.add_tag(handles.moving_rect, rain::tag_id{ "movement.mode.dynamic" });


		handles.orange_rect = create_rect(
			target_world,
			rain::string_id{"entity.orange_rect"},
			-260.0f,
			-120.0f,
			100.0f,
			100.0f,
			materials.solid_material,
			rain::sprite_color{
				.r = 1.00f,
				.g = 0.45f,
				.b = 0.25f,
				.a = 1.00f
			},
			10
		);

		handles.green_rect = create_rect(
			target_world,
			rain::string_id{"entity.green_rect"},
			240.0f,
			-80.0f,
			180.0f,
			90.0f,
			materials.image_material,
			rain::sprite_color{
				.r = 0.35f,
				.g = 1.00f,
				.b = 0.45f,
				.a = 1.00f
			},
			20
		);

		handles.white_rect = create_rect(
			target_world,
			rain::string_id{"entity.white_rect"},
			0.0f,
			0.0f,
			40.0f,
			40.0f,
			materials.image_material,
			rain::sprite_color{
				.r = 1.00f,
				.g = 1.00f,
				.b = 1.00f,
				.a = 1.00f
			},
			30
		);

		return handles;
	}
}
