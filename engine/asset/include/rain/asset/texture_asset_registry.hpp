#pragma once

#include<rain/asset/asset_id.hpp>
#include<rain/core/container/rain_hash_map.hpp>
#include<rain/render/render_backend.hpp>
#include<rain/render/render_handles.hpp>

#include<string>
#include<vector>

namespace rain {
	struct texture_asset_record {
		asset_id id;
		std::string path;
		texture_2d_handle handle;
		bool loaded = false;
        texture_format format = texture_format::rgba8_unorm;

	};

	class texture_asset_registry {
	public:
		explicit texture_asset_registry(render_backend& renderer);

		~texture_asset_registry();


		texture_asset_registry(const texture_asset_registry&) = delete;
		texture_asset_registry&operator =(const texture_asset_registry&)=delete;

		[[nodiscard]] texture_2d_handle load_texture_2d(const char* path, texture_format format = texture_format::rgba8_unorm);

		[[nodiscard]] bool is_loaded(asset_id id)const;

		bool unload_texture_2d(const char* path, texture_format format = texture_format::rgba8_unorm);
		bool unload_texture_2d(asset_id id);

		[[nodiscard]] texture_2d_handle find_texture(asset_id id)const;
		[[nodiscard]] const std::vector<texture_asset_record>& records()const;

		void clear_cache();

	private:
		render_backend* renderer_ = nullptr;
		rain_hash_map<asset_id, texture_2d_handle>texture_cache_;
		std::vector<texture_asset_record>records_;
	};
}