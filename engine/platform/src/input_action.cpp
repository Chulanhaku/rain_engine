#include<rain/platform/input_action.hpp>

#include<rain/platform/window.hpp>

#include<algorithm>

namespace rain{
	void input_action_map::bind_axis(string_id action, key_code key, f32 scale) {
		if (!action.is_valid()) {
			return;
		}

		axis_bindings_.push_back(input_axis_binding{
			.action = action,
			.key = key,
			.scale = scale
		});

		register_axis_action(action);
	}

	void input_action_map::bind_button(string_id action, key_code key) {
		if (!action.is_valid())return;

		button_bindings_.push_back(input_button_binding{
			.action = action,
			.key = key
		});

		register_button_action(action);
	}

	void input_action_map::update(const rain_window& target_window) {
		for (string_id action : axis_actions_) {
			axis_values_[action] = 0.0f;
		}

		for (const input_axis_binding& binding : axis_bindings_) {
			if (target_window.is_key_down(binding.key)) {
				axis_values_[binding.action] += binding.scale;
			}
		}

		for (string_id action : axis_actions_) {
			f32& value = axis_values_[action];

			if (value > 1.0f)value = 1.0f;
			if (value < -1.0f)value = -1.0f;

		}

		for (string_id action : button_actions_) {
			const bool* previous_current = current_button_states_.find(action);

			previous_button_states_[action] = previous_current != nullptr ? *previous_current : false;

			current_button_states_[action] = false;
		}

		for (const input_button_binding& binding : button_bindings_) {
			if (target_window.is_key_down(binding.key))current_button_states_[binding.action] = true;
		}

	}

	f32 input_action_map::get_axis(string_id action)const {
		const f32* value = axis_values_.find(action);

		if (value == nullptr) {
			return 0.0f;
		}

		return *value;
	}

	bool input_action_map::is_down(string_id action)const {
		const bool* value = current_button_states_.find(action);

		if (value == nullptr) {
			return false;
		}

		return *value;
	}

	bool input_action_map::is_pressed(string_id action) const
	{
		const bool* current = current_button_states_.find(action);
		const bool* previous = previous_button_states_.find(action);

		const bool current_value = current != nullptr ? *current : false;
		const bool previous_value = previous != nullptr ? *previous : false;

		return current_value && !previous_value;
	}

	bool input_action_map::is_released(string_id action) const
	{
		const bool* current = current_button_states_.find(action);
		const bool* previous = previous_button_states_.find(action);

		const bool current_value = current != nullptr ? *current : false;
		const bool previous_value = previous != nullptr ? *previous : false;

		return !current_value && previous_value;
	}

	void input_action_map::clear()
	{
		axis_bindings_.clear();
		button_bindings_.clear();

		axis_actions_.clear();
		button_actions_.clear();

		axis_values_.clear();
		current_button_states_.clear();
		previous_button_states_.clear();
	}

	void input_action_map::register_axis_action(string_id action) {
		if (contains_axis_action(action))return;

		axis_actions_.push_back(action);
		axis_values_[action] = 0.0f;
	}

	void input_action_map::register_button_action(string_id action) {
		if (contains_button_action(action))return;

		button_actions_.push_back(action);
		current_button_states_[action] = false;
		previous_button_states_[action] = false;

	}

	bool input_action_map::contains_axis_action(string_id action)const {
		return std::find(axis_actions_.begin(), axis_actions_.end(), action) != axis_actions_.end();
	}

	bool input_action_map::contains_button_action(string_id action)const {
		return std::find(button_actions_.begin(), button_actions_.end(), action) != button_actions_.end();
	}
}
