add_library(rain_app)

target_sources(rain_app
    PRIVATE
        src/application.cpp

    PUBLIC
        include/rain/app/application.hpp
        include/rain/app/application_context.hpp
        include/rain/app/layer.hpp
)

target_include_directories(rain_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rain_app
    PUBLIC
        rain_core
        rain_platform
        rain_runtime
        rain_gameplay
        rain_render
)

target_compile_features(rain_app
    PUBLIC
        cxx_std_20
)

rain_apply_common_options(rain_app)
rain_set_project_warnings(rain_app)