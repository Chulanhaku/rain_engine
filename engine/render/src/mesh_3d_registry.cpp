#include<rain/render/mesh_3d_registry.hpp>
#include<algorithm>
#include<cmath>

namespace {
    [[nodiscard]] rain::bounding_sphere_3d calculate_bounds(std::span<const rain::mesh_vertex_3d> vertices) {
        using namespace rain;
        if (vertices.empty()) return {};
        vec3 minimum = vertices.front().position;
        vec3 maximum = minimum;
        for (const auto& vertex : vertices) {
            minimum.x = std::min(minimum.x, vertex.position.x);
            minimum.y = std::min(minimum.y, vertex.position.y);
            minimum.z = std::min(minimum.z, vertex.position.z);
            maximum.x = std::max(maximum.x, vertex.position.x);
            maximum.y = std::max(maximum.y, vertex.position.y);
            maximum.z = std::max(maximum.z, vertex.position.z);
        }
        const vec3 center = minimum * 0.5f + maximum * 0.5f;
        f32 radius_squared = 0.0f;
        for (const auto& vertex : vertices)
            radius_squared = std::max(radius_squared, length_squared(vertex.position - center));
        return {center, std::sqrt(radius_squared)};
    }
}

namespace rain {
	mesh_3d_registry::mesh_3d_registry(render_backend& backend) : backend_(&backend) {}

	mesh_3d_registry::~mesh_3d_registry() {

	}

	mesh_3d_handle mesh_3d_registry::create(const mesh_3d_desc& desc) {
		if (desc.vertices.empty() || desc.indices.empty() ||
            std::any_of(desc.indices.begin(), desc.indices.end(),
                [&](u32 index) { return index >= desc.vertices.size(); }) ||
            std::any_of(desc.vertices.begin(), desc.vertices.end(),
                [](const mesh_vertex_3d& vertex) {
                    return !std::isfinite(vertex.position.x) || !std::isfinite(vertex.position.y) ||
                           !std::isfinite(vertex.position.z);
                })) {
			return {};
		}

		const render_buffer_handle vertex_buffer = backend_->create_vertex_buffer(render_buffer_desc{
			.name = desc.name + ".vertices",
			.bind = render_buffer_bind::vertex_buffer,
			.usage = render_buffer_usage::immutable,
			.size_bytes = desc.vertices.size_bytes(),
			.stride_bytes = sizeof(mesh_vertex_3d),
			.initial_data = desc.vertices.data()
			}
		);

		if (!backend_->is_valid(vertex_buffer)) return {};

		const render_buffer_handle index_buffer = backend_->create_index_buffer(render_buffer_desc{
			.name = desc.name + ".indices",
			.bind = render_buffer_bind::index_buffer,
			.usage = render_buffer_usage::immutable,
			.size_bytes = desc.indices.size_bytes(),
			.stride_bytes = sizeof(u32),
			.initial_data = desc.indices.data()
			}
		);


        if (!backend_->is_valid(index_buffer)) {
            backend_->destroy_render_buffer(vertex_buffer);
            return {};
        }

        return meshes_.create(mesh_3d{
			.name = desc.name,
			.vertex_buffer = vertex_buffer,
			.index_buffer = index_buffer,
			.index_count = static_cast<u32>(desc.indices.size()),
			.local_bounds = calculate_bounds(desc.vertices)
			}
		);
	}

	bool mesh_3d_registry::destroy(mesh_3d_handle handle){
		mesh_3d* mesh = meshes_.try_get(handle);

		if (mesh == nullptr) {
			return false;
		}

		backend_->destroy_render_buffer(mesh->vertex_buffer);

		backend_->destroy_render_buffer(mesh->index_buffer);

		return meshes_.destroy(handle);
	}


	mesh_3d* mesh_3d_registry::try_get(mesh_3d_handle handle) { return meshes_.try_get(handle); }

	mesh_3d_handle mesh_3d_registry::create_cube(){
		static constexpr mesh_vertex_3d vertices[] = {
			// Front
			{{-1, -1, -1}, {0, 0, -1}, {0, 1}},
			{{-1,  1, -1}, {0, 0, -1}, {0, 0}},
			{{ 1,  1, -1}, {0, 0, -1}, {1, 0}},
			{{ 1, -1, -1}, {0, 0, -1}, {1, 1}},

			// Back
			{{ 1, -1, 1}, {0, 0, 1}, {0, 1}},
			{{ 1,  1, 1}, {0, 0, 1}, {0, 0}},
			{{-1,  1, 1}, {0, 0, 1}, {1, 0}},
			{{-1, -1, 1}, {0, 0, 1}, {1, 1}},

			// Left
			{{-1, -1,  1}, {-1, 0, 0}, {0, 1}},
			{{-1,  1,  1}, {-1, 0, 0}, {0, 0}},
			{{-1,  1, -1}, {-1, 0, 0}, {1, 0}},
			{{-1, -1, -1}, {-1, 0, 0}, {1, 1}},

			// Right
			{{1, -1, -1}, {1, 0, 0}, {0, 1}},
			{{1,  1, -1}, {1, 0, 0}, {0, 0}},
			{{1,  1,  1}, {1, 0, 0}, {1, 0}},
			{{1, -1,  1}, {1, 0, 0}, {1, 1}},

			// Top
			{{-1, 1, -1}, {0, 1, 0}, {0, 1}},
			{{-1, 1,  1}, {0, 1, 0}, {0, 0}},
			{{ 1, 1,  1}, {0, 1, 0}, {1, 0}},
			{{ 1, 1, -1}, {0, 1, 0}, {1, 1}},

			// Bottom
			{{-1, -1,  1}, {0, -1, 0}, {0, 1}},
			{{-1, -1, -1}, {0, -1, 0}, {0, 0}},
			{{ 1, -1, -1}, {0, -1, 0}, {1, 0}},
			{{ 1, -1,  1}, {0, -1, 0}, {1, 1}}
		};

		static constexpr u32 indices[] = {
			0,  1,  2,  0,  2,  3,
			4,  5,  6,  4,  6,  7,
			8,  9, 10,  8, 10, 11,
			12, 13, 14, 12, 14, 15,
			16, 17, 18, 16, 18, 19,
			20, 21, 22, 20, 22, 23
		};

		return create(mesh_3d_desc{
			.name = "primitive.cube",
			.vertices = vertices,
			.indices  = indices
			}
		);
	}
}