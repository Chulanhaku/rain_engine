#pragma once

#include <rain/core/types.hpp>

namespace rain
{
    enum class key_code : i32
    {
        unknown = -1,

        space = 32,
        apostrophe = 39,
        comma = 44,
        minus = 45,
        period = 46,
        slash = 47,

        key_0 = 48,
        key_1 = 49,
        key_2 = 50,
        key_3 = 51,
        key_4 = 52,
        key_5 = 53,
        key_6 = 54,
        key_7 = 55,
        key_8 = 56,
        key_9 = 57,

        semicolon = 59,
        equal = 61,

        a = 65,
        b = 66,
        c = 67,
        d = 68,
        e = 69,
        f = 70,
        g = 71,
        h = 72,
        i = 73,
        j = 74,
        k = 75,
        l = 76,
        m = 77,
        n = 78,
        o = 79,
        p = 80,
        q = 81,
        r = 82,
        s = 83,
        t = 84,
        u = 85,
        v = 86,
        w = 87,
        x = 88,
        y = 89,
        z = 90,

        escape = 256,
        enter = 257,
        tab = 258,
        backspace = 259,
        insert = 260,
        delete_key = 261,
        right = 262,
        left = 263,
        down = 264,
        up = 265,

        left_shift = 340,
        left_control = 341,
        left_alt = 342,
        right_shift = 344,
        right_control = 345,
        right_alt = 346
    };

    enum class mouse_button : i32
    {
        button_1 = 0,
        button_2 = 1,
        button_3 = 2,
        button_4 = 3,
        button_5 = 4,
        button_6 = 5,
        button_7 = 6,
        button_8 = 7,

        left = button_1,
        right = button_2,
        middle = button_3
    };
}