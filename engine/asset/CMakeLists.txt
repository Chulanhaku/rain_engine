add_library(rain_asset)

target_sources(rain_asset
	PRIVATE
		src/image_loader.cpp
		src/texture_asset_registry.cpp
		src/gltf_model_3d.cpp
	PUBLIC
		include/rain/asset/asset_id.hpp
		include/rain/asset/image_data.hpp
		include/rain/asset/image_loader.hpp
		include/rain/asset/texture_asset_registry.hpp
		include/rain/asset/gltf_model_3d.hpp
)

target_include_directories(rain_asset
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include

	PRIVATE
		${PROJECT_SOURCE_DIR}/engine/third_party/stb
		${PROJECT_SOURCE_DIR}/engine/third_party/cgltf
)

target_link_libraries(rain_asset
	PUBLIC
		rain_render
	PRIVATE
		rain_core
)

target_compile_features(rain_asset
	PUBLIC
		cxx_std_20
)

rain_apply_common_options(rain_asset)
rain_set_project_warnings(rain_asset)