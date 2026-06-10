#include<rain/core/simd.hpp>
#include<rain/core/math/vec2.hpp>

namespace rain {
	struct alignas(16)simd_vec2 {
		simd_f32x4 value;

		simd_vec2() :value(simd_f32x4::zero()) {}

		simd_vec2(f32 x, f32 y) :value(simd_f32x4::set(x, y, 0.0f, 0.0f)) {
		}

		explicit simd_vec2(vec2 source) :value(simd_f32x4::set(source.x, source.y, 0.0f, 0.0f)) {}

		explicit simd_vec2(simd_f32x4 raw_value) :value(raw_value) {
		}

		vec2 to_vec2()const {
			f32 data[4];
			value.store(data);

			return vec2{ .x = data[0],,y = data[1] };
		}


		friend simd_vec2 operator+(simd_vec2 lhs, simd_vec2 rhs) {
			return simd_vec2(lhs.value + rhs.value);
		}
		
		friend simd_vec2 operator-(simd_vec2 lhs, simd_vec2 rhs) {
			return simd_vec2(lhs.value - rhs.value);
		}
		friend simd_vec2 operator*(simd_vec2 lhs, f32 rhs) {
			return simd_vec2(lhs.value * rhs);
		}
		friend simd_vec2 operator*(f32 lhs, simd_vec2 rhs) {
			return rhs*lhs;
		}

		friend f32 dot(simd_vec2 lhs, simd_vec2 rhs) {
			return simd_dot2(lhs.value, rhs.value);
		}
	};

	static_assert(sizeof(simd_vec2) == 16);
	static_assert(alignof(simd_vec2) == 16);
}