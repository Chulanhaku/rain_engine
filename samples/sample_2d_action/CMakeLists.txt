add_executable(sample_2d_action)

target_sources(sample_2d_action
    PRIVATE
        src/main.cpp
)

target_link_libraries(sample_2d_action
    PRIVATE
        rain_app
)

target_compile_features(sample_2d_action
    PRIVATE
        cxx_std_20
)

rain_apply_common_options(sample_2d_action)
rain_set_project_warnings(sample_2d_action)