#pragma once

#include<rain/core/string_id.hpp>
#include<rain/core/types.hpp>

#include<cstddef>
#include<functional>

namespace rain {
	struct tag_id {
		string_id name;

		tag_id() = default;

		explicit tag_id(string_id value) :name(value){

		}

		explicit tag_id(const char* value) :name(value) {

		}

		[[nodiscard]] bool is_valid()const {
			return name.is_valid();
		}

		friend bool operator==(tag_id lhs, tag_id rhs) {
			return lhs.name == rhs.name;
		}

		friend bool operator!= (tag_id lhs, tag_id rhs){
			return lhs.name != rhs.name;
		}

		friend bool operator<(tag_id lhs, tag_id rhs) {
			return lhs.name < rhs.name;
		}
	};
}
namespace std {
	template <>
	struct hash < rain::tag_id > {
		std::size_t operator()(rain::tag_id tag)const noexcept {
			return std::hash<rain::string_id>{}(tag.name);
		}
	};
}