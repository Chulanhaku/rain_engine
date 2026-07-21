add_library(rain_asset)

target_sources(rain_asset
	PRIVATE
		src/image_loader.cpp
		src/texture_asset_registry.cpp
	PUBLIC
		include/rain/asset/asset_id.hpp
		include/rain/asset/image_data.hpp
		include/rain/asset/image_loader.hpp
		include/rain/asset/texture_asset_registry.hpp
)

target_include_directories(rain_asset
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include

	PRIVATE
		${PROJECT_SOURCE_DIR}/engine/third_party/stb
)

target_link_libraries(rain_asset
	PRIVATE
		rain_core
		rain_render
)

target_compile_features(rain_asset
	PUBLIC
		cxx_std_20
)

rain_apply_common_options(rain_asset)
rain_set_project_warnings(rain_asset)