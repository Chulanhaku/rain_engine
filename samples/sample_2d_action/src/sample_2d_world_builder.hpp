#pragma once

#include<rain/runtime/entity.hpp>
#include<rain/runtime/world.hpp>

namespace sample_2d {
	struct sample_2d_world_handles {
		rain::entity_id moving_rect;
		rain::entity_id orange_rect;
		rain::entity_id green_rect;
		rain::entity_id white_rect;
	};

	[[nodiscard]] sample_2d_world_handles build_sample_2d_world(rain::world& target_world);
}