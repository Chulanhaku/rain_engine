#pragma once
#include <rain/core/types.hpp>

namespace rain {
struct fixed_step_settings {
    f32 delta_seconds = 1.0f / 120.0f;
    f32 max_frame_delta = 0.25f;
    u32 max_substeps = 30;
};
struct fixed_step_state {
    // Accumulate in double precision to avoid losing steps at frame boundaries.
    double accumulator = 0.0;
    f32 interpolation_alpha = 0.0f;
    u64 tick_index = 0;
};
}
