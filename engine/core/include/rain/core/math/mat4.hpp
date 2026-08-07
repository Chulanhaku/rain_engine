#pragma once

#include<rain/core/math/simd_vec3.hpp>
#include<rain/core/types.hpp>

#include<cmath>

namespace rain {
	struct mat4 {
		f32 values[4][4]{};

		[[nodiscard]] static mat4 identity() {
			mat4 result{};

			result.values[0][0] = 1.0f;
			result.values[1][1] = 1.0f;
			result.values[2][2] = 1.0f;
			result.values[3][3] = 1.0f;

			return result;
		}
	};

	[[nodiscard]] inline mat4 operator*(const mat4& lhs, const mat4& rhs) {
			mat4 result{};

			for (u32 row = 0; row < 4; ++row) {
				for (u32 column = 0; column < 4; ++column) {
					for (u32 index = 0; index < 4; ++index) {
						result.values[row][column] += lhs.values[row][index] * rhs.values[index][column];
					}
				}
			}


			return result;
		}


		[[nodiscard]] inline mat4 make_translation(vec3 position) {
			mat4 result = mat4::identity();
			result.values[3][0] = position.x;
			result.values[3][1] = position.y;
			result.values[3][2] = position.z;

			return result;
		}

		[[nodiscard]] inline mat4 make_scale(vec3 scale) {
			mat4 result = mat4::identity();

			result.values[0][0] = scale.x;
			result.values[1][1] = scale.y;
			result.values[2][2] = scale.z;

			return result;
		}

		[[nodiscard]] inline mat4 make_rotation_x(f32 radians) {
			const f32 cosine = std::cos(radians);
			const f32 sine = std::sin(radians);

			mat4 result = mat4::identity();

			result.values[1][1] = cosine;
			result.values[1][2] = sine;
			result.values[2][1] = -sine;
			result.values[2][2] = cosine;

			return result;
		}

		[[nodiscard]] inline mat4 make_rotation_y(f32 radians)
		{
			const f32 cosine = std::cos(radians);
			const f32 sine = std::sin(radians);

			mat4 result = mat4::identity();

			result.values[0][0] = cosine;
			result.values[0][2] = -sine;
			result.values[2][0] = sine;
			result.values[2][2] = cosine;

			return result;
		}

		[[nodiscard]] inline mat4 make_rotation_z(f32 radians)
		{
			const f32 cosine = std::cos(radians);
			const f32 sine = std::sin(radians);

			mat4 result = mat4::identity();

			result.values[0][0] = cosine;
			result.values[0][1] = sine;
			result.values[1][0] = -sine;
			result.values[1][1] = cosine;

			return result;
		}

		[[nodiscard]] inline mat4 make_transform(vec3 position, vec3 rotation, vec3 scale) {
			return make_scale(scale) * make_rotation_x(rotation.x) * make_rotation_y(rotation.y) * make_rotation_z(rotation.z) * make_translation(position);
		}

		[[nodiscard]] inline mat4 make_perspective_fov_lh(f32 vertical_fov_radians,f32 aspect_ratio,f32 near_plane,f32 far_plane) {
			const f32 y_scale = 1.0f / std::tan(vertical_fov_radians * 0.5f);

			const f32 x_scale = y_scale / aspect_ratio;

			mat4 result{};

			result.values[0][0] = x_scale;
			result.values[1][1] = y_scale;
			result.values[2][2] = far_plane / (far_plane - near_plane);
			result.values[2][3] = 1.0f;
			result.values[3][2] = -near_plane * far_plane / (far_plane - near_plane);

			return result;
		}

		[[nodiscard]] inline mat4 make_look_at_lh(vec3 eye, vec3 target, vec3 up) {
			const vec3 forward = normalize(target - eye);
			const vec3 right = normalize(cross(up, forward));
			const vec3 corrected_up = cross(forward, right);

			mat4 result = mat4::identity();

			result.values[0][0] = right.x;
			result.values[0][1] = corrected_up.x;
			result.values[0][2] = forward.x;

			result.values[1][0] = right.y;
			result.values[1][1] = corrected_up.y;
			result.values[1][2] = forward.y;

			result.values[2][0] = right.z;
			result.values[2][1] = corrected_up.z;
			result.values[2][2] = forward.z;

			result.values[3][0] = -dot(right, eye);
			result.values[3][1] = -dot(corrected_up, eye);
			result.values[3][2] = -dot(forward, eye);

			return result;
		}
		

		[[nodiscard]] inline vec3 forward_from_euler(vec3 rotation) {
			const f32 pitch_cosine = std::cos(rotation.x);

			return normalize(vec3{ .x = std::sin(rotation.y) * pitch_cosine,.y = std::sin(rotation.x),.z = std::cos(rotation.y) * pitch_cosine });
		}
}