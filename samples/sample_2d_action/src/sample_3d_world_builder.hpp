#include<rain/render/render_handles.hpp>

#include<rain/runtime/entity.hpp>
#include<rain/runtime/world.hpp>

namespace sample_3d {
	struct sample_3d_world_resources {
		rain::mesh_3d_handle cube_mesh;
		rain::material_3d_handle cube_material;
	};

	struct sample_3d_world_handles {
		rain::entity_id camera;
		rain::entity_id cube;
		rain::entity_id directional_light;
	};

	[[nodiscard]] sample_3d_world_handles build_sample_3d_world(rain::world& target_world, const sample_3d_world_resources& resources);
}