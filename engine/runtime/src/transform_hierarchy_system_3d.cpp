#include <rain/runtime/transform_hierarchy_system_3d.hpp>

#include<rain/core/tag/tag.hpp>
#include<rain/runtime/local_matrix_3d_component.hpp>
#include<rain/runtime/transform_3d_component.hpp>
#include<rain/runtime/world.hpp>
#include<rain/runtime/world_transform_3d_component.hpp>

#include<vector>

namespace rain {
	namespace {
		[[nodiscard]] mat4 local_matrix_of(world& target_world, entity_id entity) {
			const local_matrix_3d_component* matrix_override = target_world.try_get_component<local_matrix_3d_component>(entity);
		
			if (matrix_override != nullptr)return matrix_override->matrix;

			const transform_3d_component& transform = target_world.get_component<transform_3d_component>(entity);

			return transform.matrix();
		}

		void write_world_transform(world& target_world, entity_id entity, const mat4& matrix) {
			world_transform_3d_component* world_transform = target_world.try_get_component<world_transform_3d_component>(entity);

			if (world_transform == nullptr)world_transform = &target_world.add_component<world_transform_3d_component>(entity);

			world_transform->matrix = matrix;

            world_transform->position = vec3{
                matrix.values[3][0],
                matrix.values[3][1],
                matrix.values[3][2]
            };

            world_transform->right = normalize(vec3{
                matrix.values[0][0],
                matrix.values[0][1],
                matrix.values[0][2]
            });

            world_transform->up = normalize(vec3{
                matrix.values[1][0],
                matrix.values[1][1],
                matrix.values[1][2]
            });

            world_transform->forward = normalize(vec3{
                matrix.values[2][0],
                matrix.values[2][1],
                matrix.values[2][2]
            });

		}

        struct hierarchy_stack_entry {
            entity_id entity;
            mat4 parent_world = mat4::identity();
        };
	}


    void transform_hierarchy_system_3d(system_context&context,void*user_data) {
        (void)user_data;

        if (context.target_world == nullptr || context.entity_query == nullptr)return;

        world& target_world = *context.target_world;

        const entity_query_result entities = target_world.query_entities(*context.entity_query);

        if (entities.empty())return;

        std::vector<u8>candidate_flags(target_world.entity_capacity(),0);
        std::vector<u8>visited_flags(target_world.entity_capacity(), 0);

        for (entity_id entity : entities) {
            if (entity.index < candidate_flags.size()) {
                candidate_flags[entity.index] = 1;
            }
        }

        std::vector<hierarchy_stack_entry>stack;
        stack.reserve(entities.size());

        for (entity_id entity : entities) {
            const entity_id parent = target_world.parent_of(entity);

            const bool parent_is_candidate = parent.is_valid() && parent.index < candidate_flags.size() && candidate_flags[parent.index] != 0;

            const bool inherits_parent = target_world.has_tag(entity, tag_id{"transform.inherit_parent"});

            if (!parent_is_candidate || !inherits_parent) {
                stack.push_back(hierarchy_stack_entry{.entity= entity,.parent_world=mat4::identity()});
            }

        }

        while (!stack.empty()) {
            const hierarchy_stack_entry entry = stack.back();
            stack.pop_back();

            if (entry.entity.index >= visited_flags.size() || visited_flags[entry.entity.index] != 0)continue;

            visited_flags[entry.entity.index] = 1;

            const mat4 local_matrix = local_matrix_of(target_world, entry.entity);

            const mat4 world_matrix = local_matrix * entry.parent_world;
            write_world_transform(target_world, entry.entity, world_matrix);

            for (entity_id child : target_world.children_of(entry.entity)) {
                if (child.index >= candidate_flags.size() || candidate_flags[child.index] == 0)continue;


                const bool inherits_parent = target_world.has_tag(child, tag_id{ "transform.inherit_parent" });

                stack.push_back(hierarchy_stack_entry{ .entity = child,.parent_world = inherits_parent ? world_matrix : mat4::identity() });
            }
        }


        for (entity_id entity : entities) {
            if (entity.index < visited_flags.size() && visited_flags[entity.index] == 0) {
                write_world_transform(target_world, entity, local_matrix_of(target_world, entity));
            }
        }
    }
}
