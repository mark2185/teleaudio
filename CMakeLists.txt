cmake_minimum_required( VERSION 3.21 )

project(
    teleaudio
    VERSION 0.0.1
    DESCRIPTION "Streaming audio from a distance"
    LANGUAGES CXX
)

# TODO: fsanitize causes an ABI break with gRPC?
# TODO: add -Werror, but first suppress the unique_ptr warning on gcc
set( CMAKE_CXX_FLAGS "-g -Wall -std=c++20 -Wunused" )

# TODO: Run conan install automatically, or display warning if conan is not installed
# include(cmake/conan_config.cmake)

add_subdirectory(src)

add_subdirectory(test)
