#include <rain/asset/image_loader.hpp>

#include <rain/core/log.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cstring>
#include <string>
namespace rain
{
    image_data load_image_rgba8(const char* path)
    {
        image_data result;

        if (path == nullptr)
        {
            rain::log_error("load_image_rgba8 failed: path is null");
            return result;
        }

        int width = 0;
        int height = 0;
        int source_channels = 0;

        stbi_uc* pixels = stbi_load(
            path,
            &width,
            &height,
            &source_channels,
            4
        );

        if (pixels == nullptr)
        {
            rain::log_error(std::string("load_image_rgba8 failed: ") + path + ", reason: " + stbi_failure_reason());

            return result;
        }

        result.source_path = path;
        result.width = static_cast<u32>(width);
        result.height = static_cast<u32>(height);
        result.channels = 4;

        const usize size_bytes =
            static_cast<usize>(width) *
            static_cast<usize>(height) *
            4;

        result.pixels.resize(size_bytes);
        std::memcpy(result.pixels.data(), pixels, size_bytes);

        stbi_image_free(pixels);

        rain::log_info(std::string("image loaded: ") + result.source_path + " (rgba8)");

        return result;
    }
}