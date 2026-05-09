function(rain_apply_common_options target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_23)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endfunction()
