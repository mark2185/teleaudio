cmake_minimum_required( VERSION 3.21 )

project(
    teleaudio
    VERSION 0.0.1
    DESCRIPTION "Streaming audio from a distance"
    LANGUAGES CXX
)

set( CMAKE_CXX_FLAGS "-g -Wall -std=c++20" )

# TODO: Run conan install automatically, or display warning if conan is not installed
# include(cmake/conan_config.cmake)

add_subdirectory(src)

add_subdirectory(test)
