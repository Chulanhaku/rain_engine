#include<rain/asset/texture_asset_registry.hpp>
#include<rain/asset/asset_id.hpp>
#include<rain/core/log.hpp>
#include<rain/asset/image_loader.hpp>
#include<rain/render/render_resource_desc.hpp>

namespace rain {
    texture_asset_registry::texture_asset_registry(render_backend& renderer)
        : renderer_(&renderer)
    {
    }

    texture_2d_handle texture_asset_registry::load_texture_2d(const char* path) {
        if (path == nullptr) {
            rain::log_error("texture_asset_registry::failed path is null");
            return texture_2d_handle{};
        }

        const asset_id id(path);

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
                .loaded = false
                });
            return texture_2d_handle{};
        }

        texture_2d_handle texture = renderer_->create_texture_2d(texture_2d_desc{
            .name = image.source_path,
            .width = image.width,
            .height = image.height,
            .format = texture_format::rgba8_unorm,
            .pixels = image.pixels.data(),
            .size_bytes = image.size_bytes()
            });

        texture_cache_[id] = texture;

        records_.push_back(texture_asset_record{
            .id = id,
            .path = image.source_path,
            .handle = texture,
            .loaded = texture.is_valid()
            });


        rain::log_info("texture asset loaded");

        return texture;
    }

    bool texture_asset_registry::is_loaded(asset_id id)const {
        const texture_2d_handle* texture = texture_cache_.find(id);
        return texture != nullptr && texture->is_valid();
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
        texture_cache_.clear();
        records_.clear();

    }
}