#pragma once

#include <rain/core/types.hpp>

namespace rain {
	enum class render_blend_mode :u8 {
		opaque,
		alpha,
		additive
	};

	enum class render_index_format :u8 {
		uint16,
		uint32
	};

	enum class render_cull_mode :u8 {
		none,
		front,
		back
	};

	enum class render_compare_operation :u8 {
		less,
		less_equal,
		always
	};
}