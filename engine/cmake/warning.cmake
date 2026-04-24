function(rain_set_project_warnings target_name)
    if(CMAKE_CXX_COMPILE_ID STREQUAL "GNU")
        target_compile_options(${target_name}PRIVATE
            -Wall -Wextra -Wshadow -Wconversion
        )
    elseif(CMAKE_CXX_COMPILE_ID MATCHES "Clang")
        target_compile_options(${target_name}PRIVATE
            -Wall -Wextra -Wshadow -Wconversion
        )