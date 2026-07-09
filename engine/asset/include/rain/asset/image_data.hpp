#pragma once

#include <rain/core/types.hpp>

#include <string>
#include <vector>

namespace rain
{
    struct image_data
    {
        std::string source_path;

        u32 width = 0;
        u32 height = 0;
        u32 channels = 4;

        std::vector<u8> pixels;

        [[nodiscard]] bool is_valid() const
        {
            return width > 0 &&
                   height > 0 &&
                   channels == 4 &&
                   !pixels.empty();
        }

        [[nodiscard]] usize size_bytes() const
        {
            return pixels.size();
        }
    };
}