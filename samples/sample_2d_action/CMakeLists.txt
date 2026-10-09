add_executable(sample_2d_action)

target_sources(sample_2d_action
    PRIVATE
        src/main.cpp
        src/sample_physics_3d.cpp
        src/sample_physics_world_tests.cpp
        src/sample_physics_3d.hpp
        src/sample_2d_world_builder.cpp
        src/sample_2d_world_builder.hpp
        src/sample_3d_world_builder.cpp
        src/sample_3d_world_builder.hpp
)

target_link_libraries(sample_2d_action
    PRIVATE
        rain_app
        rain_asset
)

target_compile_features(sample_2d_action
    PRIVATE
        cxx_std_20
)

rain_apply_common_options(sample_2d_action)
rain_set_project_warnings(sample_2d_action)
if(rain_build_tests)
    add_test(NAME rain_physics_v0_regressions COMMAND sample_2d_action --physics-test)
    set_tests_properties(rain_physics_v0_regressions PROPERTIES
        TIMEOUT 30
        LABELS "physics;physics_world;sample"
    )
endif()
