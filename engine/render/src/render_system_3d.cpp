#include<rain/render/render_system_3d.hpp>

namespace rain{
	camera_3d_frame build_camera_frame(const transform_3d_component&transform,const camera_3d_component&camera,f32 aspect_ratio) {
		const vec3 forward = forward_from_euler(transform.rotation);

		camera_3d_frame result{};

		result.position = transform.position;

		result.view = make_look_at_lh(transform.position, transform.position + forward, vec3{0.0f,1.0f,0.0f});

		result.projection = make_perspective_fov_lh(camera.vertical_fov_radians,aspect_ratio,camera.near_plane,camera_far_plane);

		return result;
	}


	entity_query_desc render_system_3d::make_render_query(){
		tag_query tags;

		tags.require_all(tag_id{ "object.renderable" });
		tags.require_all(tag_id{"render.3d"});
		tags.reject(tag_id{"render.hidden"});

		return entity_query_desc{
			.required_components = {
				get_type_id<transform_3d_component>(),
				get_type_id<mesh_3d_component>()
			},
			.require_tags = tags,
			.require_alive = true,
			.require_active = true
		};

	
	}


	entity_query_desc render_system_3d::make_camera_query() {
		tag_query tags;

		tags.require_all(tag_id{"camera.3d"});
		tags.require_all(tag_id{ "camera.active" });

		return entity_query_desc{
			.required_components = {
				get_type_id<transform_3d_component>(),
				get_type_id<camera_3d_component>()
			},
			.require_tags = tags,
			.require_alive = true,
			.require_active = true
		};
	}

	entity_query_desc render_system_3d::make_light_query() {
		tag_query tags;

		tags.require_all(tag_id{ "light.directional" });
		tags.require_all(tag_id{ "light.active" });

		return entity_query_desc{
			.required_components = {
				get_type_id<directional_light_3d_component>(),
			},
			.require_tags = tags,
			.require_alive = true,
			.require_active = true
		};
	}


	void render_system_3d::prepare(world& target_world) {
		queue_.begin_frame();

		const entity_query_result cameras = target_world.query_entities(camera_query_);

		if (camera.empty()) {
			has_camera_ = false;
			return;
		}

		const entity_id camera_entity = cameras.entities.front();

		const transform_3d_component& camera_transform = target_world.get_component<transform_3d_component>(camera_entity);

		const f32 aspect_ratio = static_cast<f32>(backend_->width() / static_cast<f32>(backend_->height()));

		camera_frame_ = build_camera_frame(camera_transform,camera,aspect_ratio);

		has_camera_ = true;

		directionnal_3d_component light{};

		const entity_query_result lights = target_world.query_entities(light_query_);

		if (!lights.empty()) {
			light = target_world.get_component<directonal_light_3d_component>(lights.entities.front());
		}

		light_ = light;

		const entity_query_result renderables = target_world.query_entities(render_query_);

		for (entity_id entity : renderables) {
			const transform_3d_component& transform = target_world.get_component<transform_3d_component>(entity);

			const mesh_3d_component& mesh = target_world.get_component<mesh_3d_component>(entity);

			const vec3 difference = transform.position - camera_frame_.position;

			queue_.push(render_command_3d{
				.source_entity = entity,
				.world_matrix = transform.matrix,
				.mesh = mesh.mesh,
				.material = mesh.material,
				.tint = mesh.tint,
				.camera_distance_squared = length_squared(difference),
				.layer = mesh.layer,
				}
			);
		}

		queue_.sort(*materials_);
	}


	void render_prepare_system_3d(system_context& context, void* user_data) {
		if (context.target_world == nullptr || user_data == nullptr)return;

		auto* render_system = static_cast<render_system_3d*>(user_data);

		render_system->prepare(*context.target_world);
	}
}