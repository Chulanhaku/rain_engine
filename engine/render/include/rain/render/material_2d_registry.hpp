#pragma once

#include<rain/core/container/handle_pool.hpp>
#include<rain/render/material_2d.hpp>
#include<rain/render/render_handles.hpp>

namespace rain {
	class material_2d_registry {
	public:
		material_2d_registry();
		[[nodiscard]] material_2d_handle create(const material_2d_desc& desc);
		bool destroy(material_2d_handle handle);
		[[nodiscard]] bool is_valid(material_2d_handle handle)const;

		[[nodiscard]] material_2d& get(material_2d_handle handle);
		[[nodiscard]] const material_2d& get(material_2d_handle handle)const;

		[[nodiscard]] material_2d* try_get(material_2d_handle handle);
		[[nodiscard]] const material_2d* try_get(material_2d_handle handle)const;

		[[nodiscard]] material_2d_handle default_material()const;
		[[nodiscard]] usize size()const;

	private:
		handle_pool<material_2d_handle_tag, material_2d>materials_;
		material_2d_handle default_material_;
	};
}