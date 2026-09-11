#include<rain/asset/texture_asset_registry.hpp>
#include<rain/asset/asset_id.hpp>
#include<rain/core/log.hpp>
#include<rain/asset/image_loader.hpp>
#include<rain/render/render_resource_desc.hpp>

namespace rain {
    namespace {
        asset_id texture_cache_id(const char* path, texture_format format) {
            // Preserve the legacy ID for linear textures used by 2D callers.
            return format == texture_format::rgba8_unorm ? asset_id{path}
                : asset_id{(std::string{path} + "#rain.texture.srgb").c_str()};
        }
    }

    texture_asset_registry::texture_asset_registry(render_backend& renderer)
        : renderer_(&renderer)
    {
    }

    texture_asset_registry::~texture_asset_registry() {
        clear_cache();
    }

    texture_2d_handle texture_asset_registry::load_texture_2d(const char* path, texture_format format) {
        if (path == nullptr) {
            rain::log_error("texture_asset_registry::failed path is null");
            return texture_2d_handle{};
        }

        const asset_id id = texture_cache_id(path, format);

        if (!id.is_valid()) {
            rain::log_error("asset registry failed: id invalid");
            return texture_2d_handle{};
        }

        const texture_2d_handle* cached_texture = texture_cache_.find(id);

        if (cached_texture != nullptr) {
            return *cached_texture;
        }

        const image_data image = load_image_rgba8(path);

        if(!image.is_valid()){
            records_.push_back(texture_asset_record{
                .id = id,
                .path = path,
                .handle = texture_2d_handle{},
                .loaded = false,
                .format = format
                });
            return texture_2d_handle{};
        }

        texture_2d_handle texture = renderer_->create_texture_2d(texture_2d_desc{
            .name = image.source_path,
            .width = image.width,
            .height = image.height,
            .format = format,
            .pixels = image.pixels.data(),
            .size_bytes = image.size_bytes()
            });

        texture_cache_[id] = texture;

        records_.push_back(texture_asset_record{
            .id = id,
            .path = image.source_path,
            .handle = texture,
            .loaded = texture.is_valid(),
                .format = format
            });


        rain::log_info("texture asset loaded");

        return texture;
    }

    bool texture_asset_registry::is_loaded(asset_id id)const {
        const texture_2d_handle* texture = texture_cache_.find(id);
        return texture != nullptr && texture->is_valid();
    }


    bool texture_asset_registry::unload_texture_2d(const char* path, texture_format format) {
        if (path == nullptr) {
            return false;
        }

        return unload_texture_2d(texture_cache_id(path, format));
    }

    bool texture_asset_registry::unload_texture_2d(asset_id id) {
        texture_2d_handle* texture = texture_cache_.find(id);

        if (texture == nullptr)return false;

        const texture_2d_handle handle = *texture;

        texture_cache_.erase(id);

        for (texture_asset_record& record : records_) {
            if (record.id != id) {
                continue;
            }

            record.loaded = false;
            record.handle = texture_2d_handle{};
        }

        if (!handle.is_valid())return true;

        return renderer_->destroy_texture_2d(handle);
    }

    texture_2d_handle texture_asset_registry::find_texture(asset_id id)const {
        const texture_2d_handle* texture = texture_cache_.find(id);

        if (texture == nullptr) {
            return texture_2d_handle{};
        }

        return *texture;
    }

    const std::vector<texture_asset_record>& texture_asset_registry::records()const {
        return records_;
    }

    void texture_asset_registry::clear_cache() {
        if (renderer_ != nullptr) {
            for (texture_asset_record& record : records_) {
                if (!record.loaded)continue;
                if (!record.handle.is_valid())continue;
                renderer_->destroy_texture_2d(record.handle);
                record.loaded = false;
                record.handle = texture_2d_handle{};
            }
        }

        texture_cache_.clear();
        records_.clear();

    }
}