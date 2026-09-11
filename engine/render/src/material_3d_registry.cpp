#include <rain/render/material_3d_registry.hpp>

namespace rain {
    material_3d_registry::material_3d_registry() {
        default_material_ = materials_.create(material_3d{
            .name = "material.default_3d",
            .metallic_factor = 0.0f,
            .roughness_factor = 0.8f,
            .alpha_cutoff = -1.0f
        });
    }

    material_3d_handle material_3d_registry::create(const material_3d_desc& desc) {
        return materials_.create(desc);
    }

    bool material_3d_registry::destroy(material_3d_handle handle) {
        if (handle == default_material_) {
            return false;
        }
        return materials_.destroy(handle);
    }

    material_3d* material_3d_registry::try_get(material_3d_handle handle) {
        return materials_.try_get(handle);
    }

    const material_3d* material_3d_registry::try_get(material_3d_handle handle) const {
        return materials_.try_get(handle);
    }

    material_3d_handle material_3d_registry::default_material() const {
        return default_material_;
    }
}
