#pragma once

#include<rain/core/types.hpp>

namespace rain {

	struct vec2 {
		f32 x = 0.0f;
		f32 y = 0.0f;

		friend vec2 operator+(vec2 lhs, vec2 rhs) {
			return vec2{
				.x = lhs.x +rhs.x,
				.y = lhs.y +rhs.y
			};
		}

        friend vec2 operator-(vec2 lhs, vec2 rhs)
        {
            return vec2{
                .x = lhs.x - rhs.x,
                .y = lhs.y - rhs.y
            };
        }

        friend vec2 operator*(vec2 lhs, f32 rhs)
        {
            return vec2{
                .x = lhs.x * rhs,
                .y = lhs.y * rhs
            };
        }

        friend vec2 operator*(f32 lhs, vec2 rhs)
        {
            return rhs * lhs;
        }
	};
}