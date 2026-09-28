#pragma once 
#include<rain/core/types.hpp>

namespace rain {
	struct fixed_step_settings {
		f32 delta_seconds = 1.0f / 60.0f;

		f32 max_frame_delta = 0.25f;

		u32 max_substeps = 0;
	};

	struct fixed_step_state {
		f32 accumulator = 0.0f;
		f32 interpolation_alpha = 0.0f;
		u64 tick_index = 0;
	};

}