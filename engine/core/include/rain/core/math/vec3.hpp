#pragma once

#include <rain/core/types.hpp>

#include <cmath>

namespace rain
{
    struct vec3
    {
        f32 x = 0.0f;
        f32 y = 0.0f;
        f32 z = 0.0f;
    };

    [[nodiscard]] inline vec3 operator+(vec3 lhs, vec3 rhs)
    {
        return {
            lhs.x + rhs.x,
            lhs.y + rhs.y,
            lhs.z + rhs.z
        };
    }

    [[nodiscard]] inline vec3 operator-(vec3 lhs, vec3 rhs)
    {
        return {
            lhs.x - rhs.x,
            lhs.y - rhs.y,
            lhs.z - rhs.z
        };
    }

    [[nodiscard]] inline vec3 operator*(vec3 value, f32 scalar)
    {
        return {
            value.x * scalar,
            value.y * scalar,
            value.z * scalar
        };
    }

    [[nodiscard]] inline vec3 operator*(f32 scalar, vec3 value)
    {
        return value * scalar;
    }

    [[nodiscard]] inline f32 dot(vec3 lhs, vec3 rhs)
    {
        return lhs.x * rhs.x +
               lhs.y * rhs.y +
               lhs.z * rhs.z;
    }

    [[nodiscard]] inline vec3 cross(vec3 lhs, vec3 rhs)
    {
        return {
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x
        };
    }

    [[nodiscard]] inline f32 length_squared(vec3 value)
    {
        return dot(value, value);
    }

    [[nodiscard]] inline f32 length(vec3 value)
    {
        return std::sqrt(length_squared(value));
    }

    [[nodiscard]] inline vec3 normalize(vec3 value)
    {
        const f32 value_length = length(value);

        if (value_length <= 0.000001f)
        {
            return {};
        }

        const f32 inverse_length = 1.0f / value_length;

        return value * inverse_length;
    }


    [[nodiscard]] inline vec3 operator-(vec3 value) {
        return {
            -value.x,
            -value.y,
            -value.z
        };
    }

    [[nodiscard]] inline vec3 operator/(vec3 value, f32 scalar) {
        return {
            value.x / scalar,
            value.y / scalar,
            value.z / scalar
        };
    }

    inline vec3& operator+=(vec3& lhs, vec3 rhs) {
        lhs.x += rhs.x;
        lhs.y += rhs.y;
        lhs.z += rhs.z;

        return lhs;
    }

    inline vec3& operator-=(vec3& lhs, vec3 rhs) {
        lhs.x -= rhs.x;
        lhs.y -= rhs.y;
        lhs.z -= rhs.z;

        return lhs;
    }

    inline vec3& operator *=(vec3& lhs, f32 scalar) {
        lhs.x *= scalar;
        lhs.y *= scalar;
        lhs.z *= scalar;
        return lhs;
    }
}