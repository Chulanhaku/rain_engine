#pragma once

#include <rain/core/math/vec3.hpp>
#include <rain/core/simd.hpp>

#include <cmath>

namespace rain
{
    struct alignas(16) simd_vec3
    {
        simd_f32x4 value;

        simd_vec3()
            : value(simd_f32x4::zero())
        {
        }

        simd_vec3(f32 x, f32 y, f32 z)
            : value(simd_f32x4::set(x, y, z, 0.0f))
        {
        }

        explicit simd_vec3(vec3 source)
            : simd_vec3(source.x, source.y, source.z)
        {
        }

        explicit simd_vec3(simd_f32x4 raw_value)
        {
            f32 data[4];
            raw_value.store(data);
            value = simd_f32x4::set(data[0], data[1], data[2], 0.0f);
        }

        [[nodiscard]] vec3 to_vec3() const
        {
            f32 data[4];
            value.store(data);

            return vec3{
                .x = data[0],
                .y = data[1],
                .z = data[2]
            };
        }
    };

    [[nodiscard]] inline simd_vec3 operator+(
        simd_vec3 lhs,
        simd_vec3 rhs)
    {
        return simd_vec3(lhs.value + rhs.value);
    }

    [[nodiscard]] inline simd_vec3 operator-(
        simd_vec3 lhs,
        simd_vec3 rhs)
    {
        return simd_vec3(lhs.value - rhs.value);
    }

    [[nodiscard]] inline simd_vec3 operator*(
        simd_vec3 value,
        f32 scalar)
    {
        return simd_vec3(value.value * scalar);
    }

    [[nodiscard]] inline simd_vec3 operator*(
        f32 scalar,
        simd_vec3 value)
    {
        return value * scalar;
    }

    [[nodiscard]] inline f32 dot(simd_vec3 lhs, simd_vec3 rhs)
    {
        return simd_dot3(lhs.value, rhs.value);
    }

    [[nodiscard]] inline simd_vec3 cross(simd_vec3 lhs, simd_vec3 rhs)
    {
        return simd_vec3(simd_cross3(lhs.value, rhs.value));
    }

    [[nodiscard]] inline f32 length_squared(simd_vec3 value)
    {
        return dot(value, value);
    }

    [[nodiscard]] inline f32 length(simd_vec3 value)
    {
        return std::sqrt(length_squared(value));
    }

    [[nodiscard]] inline simd_vec3 normalize(simd_vec3 value)
    {
        const f32 value_length = length(value);

        if (value_length <= 0.000001f)
        {
            return {};
        }

        return value * (1.0f / value_length);
    }

    static_assert(sizeof(simd_vec3) == 16);
    static_assert(alignof(simd_vec3) == 16);
}