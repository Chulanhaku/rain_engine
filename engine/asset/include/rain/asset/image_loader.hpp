#pragma once

#include <rain/asset/image_data.hpp>

namespace rain {
	[[nodiscard]] image_data load_image_rgba8(const char* path);
}