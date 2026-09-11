#pragma once

#include<rain/core/math/simd_vec3.hpp>
#include<rain/core/types.hpp>

#include<algorithm>
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


		[[nodiscard]] inline mat4 make_normal_matrix(const mat4& world) {
			const f32 a00 = world.values[0][0];
			const f32 a01 = world.values[0][1];
			const f32 a02 = world.values[0][2];

			const f32 a10 = world.values[1][0];
			const f32 a11 = world.values[1][1];
			const f32 a12 = world.values[1][2];

			const f32 a20 = world.values[2][0];
			const f32 a21 = world.values[2][1];
			const f32 a22 = world.values[2][2];

			const f32 determinant = a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);

			if (std::abs(determinant) <= 0.000001f)return mat4::identity();

			const f32 inverse_determinant = 1.0f / determinant;

			mat4 result = mat4::identity();

			result.values[0][0] =
				(a11 * a22 - a12 * a21) *
				inverse_determinant;

			result.values[0][1] =
				(a12 * a20 - a10 * a22) *
				inverse_determinant;

			result.values[0][2] =
				(a10 * a21 - a11 * a20) *
				inverse_determinant;

			result.values[1][0] =
				(a02 * a21 - a01 * a22) *
				inverse_determinant;

			result.values[1][1] =
				(a00 * a22 - a02 * a20) *
				inverse_determinant;

			result.values[1][2] =
				(a01 * a20 - a00 * a21) *
				inverse_determinant;

			result.values[2][0] =
				(a01 * a12 - a02 * a11) *
				inverse_determinant;

			result.values[2][1] =
				(a02 * a10 - a00 * a12) *
				inverse_determinant;

			result.values[2][2] =
				(a00 * a11 - a01 * a10) *
				inverse_determinant;

			result.values[3][0] = 0.0f;
			result.values[3][1] = 0.0f;
			result.values[3][2] = 0.0f;

			return result;
		}

		[[nodiscard]] inline vec3 transform_point(vec3 point, const mat4& matrix) {
			const f32 x = point.x * matrix.values[0][0] +
				point.y * matrix.values[1][0] +
				point.z * matrix.values[2][0] +
				matrix.values[3][0];

			const f32 y = point.x * matrix.values[0][1] +
				point.y * matrix.values[1][1] +
				point.z * matrix.values[2][1] +
				matrix.values[3][1];

			const f32 z = point.x * matrix.values[0][2] +
				point.y * matrix.values[1][2] +
				point.z * matrix.values[2][2] +
				matrix.values[3][2];

			const f32 w = point.x * matrix.values[0][3] +
				point.y * matrix.values[1][3] +
				point.z * matrix.values[2][3] +
				matrix.values[3][3];

			if (std::abs(w) > 0.000001f) {
				const f32 inverse_w = 1.0f / w;
				return { x * inverse_w,y * inverse_w,z * inverse_w };
			}

			return { x,y,z };
		}

		[[nodiscard]] inline f32 max_basis_scale(const mat4& matrix) {
			const vec3 axis_x{matrix.values[0][0],matrix.values[0][1],matrix.values[0][2]};

			const vec3 axis_y{ matrix.values[1][0],matrix.values[1][1],matrix.values[1][2] };

			const vec3 axis_z{ matrix.values[2][0],matrix.values[2][1],matrix.values[2][2] };

			return std::max(length(axis_x), std::max(length(axis_y), length(axis_z)));
		}
}
