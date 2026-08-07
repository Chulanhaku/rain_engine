#pragma once

#include<rain/core/container/handle_pool.hpp>
#include<rain/render/material_3d.hpp>
#include<rain/render/render_handles.hpp>

namespace rain {

	class material_3d_registry {
	public:
		material_3d_registry();

		material_3d_handle create(const material_3d_desc& desc);

		bool destroy(material_3d_handle handle);

		material_3d* try_get(material_3d_handle handle);

		const material_3d* try_get(material_3d_handle handle)const;

		material_3d_handle default_material()const;
	private:
		handle_pool<material_3d_handle_tag, material_3d>materials_;

		material_3d_handle default_material_;
	};

}