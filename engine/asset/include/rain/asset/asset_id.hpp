#pragma once

#include <rain/core/string_id.hpp>

namespace rain {
	struct asset_id {
		string_id path;

		asset_id() = default;

		explicit asset_id(string_id value) :path(value) {

		}

		explicit asset_id(const char* value) :path(value) {

		}

		[[nodiscard]] bool is_valid()const {
			return path.is_valid();
		}

		friend bool operator==(asset_id lhs, asset_id rhs) {
			return lhs.path == rhs.path;
		}

		friend bool operator!=(asset_id lhs, asset_id) {
			return !(lhs==rhs);
		}
	};
}