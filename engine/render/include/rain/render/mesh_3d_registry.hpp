#pragma once

#include<rain/core/container/handle_pool.hpp>
#include<rain/render/mesh_3d.hpp>
#include<rain/render/render_backend.hpp>

namespace rain {
	class mesh_3d_registry {
	public:
		explicit mesh_3d_registry(render_backend& backend);

		~mesh_3d_registry();

		[[nodiscard]] mesh_3d_handle create(const mesh_3d_desc& desc);

		bool destory(mesh_3d_handle handle);

		[[nodiscard]] mesh_3d* try_get(mesh_3d_handle handle);

		[[nodiscard]] mesh_3d_handle create_cube();

	private:
		render_backend* backend  = nullptr;
		handle_pool<mesh_3d_handle_tag,mesh_3d>meshes_;
	};
}