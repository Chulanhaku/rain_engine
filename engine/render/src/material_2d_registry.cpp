#include <rain/render/material_2d_registry.hpp>

#include<utility>

namespace rain {
	material_2d_registry::material_2d_registry(){
		default_material_ = materials_.create(material_2d{
			.name = "default_material_2d",
			.texture = texture_2d_handle{},
			.blend_mode = render_blend_mode::alpha
			});

	}

	material_2d_handle material_2d_registry::create(const material_2d_desc & desc) {
		return materials_.create(material_2d{
			.name = desc.name,
			.texture = desc.texture,
			.blend_mode = desc.blend_mode
			});
	}

	bool material_2d_registry::destroy(material_2d_handle handle) {
		if (handle == default_material_) {
			return false;
		}

		return materials_.destroy(handle);
	}

	bool material_2d_registry::is_valid(material_2d_handle handle)const {
		return materials_.is_valid(handle);
	}

	material_2d& material_2d_registry::get(material_2d_handle handle){
		return materials_.get(handle);
	}

	material_2d* material_2d_registry::try_get(material_2d_handle handle) {
		return materials_.try_get(handle);
	}

	const material_2d& material_2d_registry::get(material_2d_handle handle)const {
		return materials_.get(handle);
	}

	const material_2d* material_2d_registry::try_get(material_2d_handle handle)const {
		return materials_.try_get(handle);
	}

	material_2d_handle material_2d_registry::default_material()const {
		return default_material_;
	}

	usize material_2d_registry::size() const {
		return materials_.size();
	}

}