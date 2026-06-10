#pragma once

#include<rain/core/types.hpp>

#if defined(__SSE__)||defined(_M_X64)||(defined(_M_IX86_FP)&&_M_IX86_FP>=1)
	#define RAIN_HAS_SSE 1
	#include<xmmintrin.h>
#else
	#define RAIN_HAS_SSE 0
#endif

namespace rain {

	struct alignas(16) simd_f32x4 {
#if RAIN_HAS_SSE
		__m128 value;

		simd_f32x4():value(_mm_setzero_ps()){}

		explicit simd_f32x4(__m128 native_value):value(native_value){}

		static simd_f32x4 zero() {
			return simd_f32x4(_mm_setzero_ps());
		}

		static simd_f32x4 splat(f32 value) {
			return simd_f32x4(_mm_set1_ps(value));
		}

		static simd_f32x4 set(f32 x, f32 y, f32 z, f32 w) {
			return simd_f32x4(_mm_set_ps(w, z, y, x));
		}

		void store(f32* out)const {
			_mm_storeu_ps(out, value);
		}
#else
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 z = 0.0f;
		f32 w = 0.0f

		simd_f32x4() = default;

		simd_f32x4(f32 in_x,f32 in_y,f32 in_z,f32 in_w):x(in_x),y(in_y),z(in_z),w(in_w){}

		static simd_f32x4 zero() {
			return simd_f32x4{};
		}

		static simd_f32x4 splat(f32 value) {
			return simd_f32x4{
				value,
				value,
				value,
				value
			};
		}

		static simd_f32x4 set(f32 x, f32 y, f32 z, f32 w) {
			return simd_f32x4{
				x,y,z,w
			};
		}

		void store(f32* out)const {
			out[0] = x;
			out[1] = y;
			out[2] = z;
			out[3] = w;
		}
#endif
		
	};

	static_assert(sizeof(simd_f32x4) == 16);
	static_assert(alignof(simd_f32x4) == 16);

	inline simd_f32x4 operator+(simd_f32x4 lhs, simd_f32x4 rhs)
	{
#if RAIN_HAS_SSE
		return simd_f32x4(_mm_add_ps(lhs.value, rhs.value));
#else
		return simd_f32x4::set(
			lhs.x + rhs.x,
			lhs.y + rhs.y,
			lhs.z + rhs.z,
			lhs.w + rhs.w
		);
#endif
	}

	inline simd_f32x4 operator-(simd_f32x4 lhs, simd_f32x4 rhs)
	{
#if RAIN_HAS_SSE
		return simd_f32x4(_mm_sub_ps(lhs.value, rhs.value));
#else
		return simd_f32x4::set(
			lhs.x - rhs.x,
			lhs.y - rhs.y,
			lhs.z - rhs.z,
			lhs.w - rhs.w
		);
#endif
	}

	inline simd_f32x4 operator*(simd_f32x4 lhs, simd_f32x4 rhs)
	{
#if RAIN_HAS_SSE
		return simd_f32x4(_mm_mul_ps(lhs.value, rhs.value));
#else
		return simd_f32x4::set(
			lhs.x * rhs.x,
			lhs.y * rhs.y,
			lhs.z * rhs.z,
			lhs.w * rhs.w
		);
#endif
	}

	inline simd_f32x4 operator*(simd_f32x4 lhs, f32 rhs)
	{
		return lhs * simd_f32x4::splat(rhs);
	}

	inline simd_f32x4 operator*(f32 lhs, simd_f32x4 rhs)
	{
		return rhs * lhs;
	}

	inline f32 simd_dot2(simd_f32x4 lhs, simd_f32x4 rhs)
	{
#if RAIN_HAS_SSE
		const __m128 multiplied = _mm_mul_ps(lhs.value, rhs.value);

		const __m128 shuffled = _mm_shuffle_ps(
			multiplied,
			multiplied,
			_MM_SHUFFLE(2, 3, 0, 1)
		);

		const __m128 result = _mm_add_ss(multiplied, shuffled);

		return _mm_cvtss_f32(result);
#else
		return lhs.x * rhs.x + lhs.y * rhs.y;
#endif
	}



}