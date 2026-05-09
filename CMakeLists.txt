cmake_minimum_required(VERSION 3.25)

project(rain_engine
    VERSION 0.1.0 
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/standard_project_settings.cmake)
include(cmake/compiler_options.cmake)
include(cmake/warnings.cmake)

option(rain_build_editor "build rain editor" ON)
option(rain_build_samples "build samples" ON)
option(rain_build_tests "build tests" ON)

add_subdirectory(engine)

if(rain_build_editor)
    add_subdirectory(editor)
endif()

if(rain_build_samples)
    add_subdirectory(smamples)
endif()

if(rain_build_tests)
    add_subdirectory(tests)
endif()

