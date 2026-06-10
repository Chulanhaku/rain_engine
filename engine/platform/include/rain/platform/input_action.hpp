#pragma once

#include<rain/core/container/rain_hash_map.hpp>

#include<rain/core/string_id.hpp>
#include<rain/core/types.hpp>
#include<rain/platform/key_code.hpp>

#include<vector>

namespace rain {
	struct input_axis_binding {
		string_id action;
		key_code key = key_code::unknown;
		f32 scale = 1.0f;
	};

	struct input_button_binding {
		string_id action;
		key_code key = key_code::unknown;
	};

	class rain_window;

	class input_action_map {
	public:
		void bind_axis(string_id action, key_code key, f32 scale);
		void bind_button(string_id action, key_code key);
		
		void update(const rain_window& target_window);

		[[nodiscard]] f32 get_axis(string_id action)const;
		[[nodiscard]] bool is_down(string_id action)const;
		[[nodiscard]] bool is_pressed(string_id action)const;
		[[nodiscard]] bool is_released(string_id action)const;

		void clear();

	private:
		void register_axis_action(string_id action);
		void register_button_action(string_id action);

		[[nodiscard]] bool contains_axis_action(string_id action)const;
		[[nodiscard]] bool contains_button_action(string_id action)const;

	private:
		std::vector<input_axis_binding> axis_bindings_;
		std::vector<input_button_binding>button_bindings_;

		std::vector<string_id>axis_actions_;
		std::vector<string_id>button_actions_;

		rain_hash_map<string_id, f32>axis_values_;
		rain_hash_map<string_id, bool>current_button_states_;
		rain_hash_map<string_id, bool>previous_button_states_;
	};
}
