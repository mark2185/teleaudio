cmake_minimum_required( VERSION 3.21 )

project(
    teleaudio
    VERSION 0.0.1
    DESCRIPTION "Streaming audio from a distance"
    LANGUAGES CXX
)

# TODO: fsanitize causes an ABI break with gRPC?
set( CMAKE_CXX_FLAGS "-Wall -std=c++20 -Wunused -Werror -Wextra -Wconversion -Wno-ignored-attributes -fPIC" )

add_subdirectory(src)

add_subdirectory(test)
