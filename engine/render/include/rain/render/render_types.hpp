#pragma once

#include <rain/core/types.hpp>

namespace rain {
	enum class render_blend_mode :u8 {
		opaque,
		alpha,
		additive
	};
}