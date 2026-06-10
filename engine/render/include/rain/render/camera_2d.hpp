#pragma once

#include<rain/core/math/vec2.hpp>
#include<rain/core/types.hpp>

namespace rain {
	struct camera_2d_desc {
		vec2 position{ 0.0f,0.0f };

		f32 viewport_width = 1280.0f;
		f32 viewport_height = 720.0f;

		f32 zoom = 1.0f;
	};

	class camera_2d {
	public:
		camera_2d() = default;

		explicit camera_2d(const camera_2d_desc& desc) :position_(desc.position), viewport_width_(desc.viewport_width), viewport_height_(desc.viewport_height), zoom_(desc.zoom) {

		}

		void set_position(vec2 position) {
			position_ = position;
		}

		void set_viewport_size(f32 width, f32 height) {
			viewport_width_ = width;
			viewport_height_ = height;
		}

		void set_zoom(f32 zoom) {
			if (zoom <= 0.0f) {
				return;
			}

			zoom_ = zoom;
		}

		[[nodiscard]] vec2 position()const {
			return position_;
		}

		[[nodiscard]] f32 viewport_width()const {
			return viewport_width_;
		}

		[[nodiscard]] f32 viewport_height()const {
			return viewport_height_;
		}

		[[nodiscard]] f32 zoom()const {
			return zoom_;
		}

		[[nodiscard]] vec2 world_to_ndc(vec2 world_position)const {
			const f32 half_width = viewport_width_ * 0.5f / zoom_;
			const f32 half_height = viewport_height_ * 0.5f / zoom_;

			return vec2{ 
				.x = (world_position.x - position_.x) / half_width,
				.y = (world_position.y - position_.y) / half_height,
			};
		}

	private:
		vec2 position_{ 0.0f, 0.0f };

		f32 viewport_width_ = 1280.0f;
		f32 viewport_height_ = 720.0f;

		f32 zoom_ = 1.0f;
	};
}