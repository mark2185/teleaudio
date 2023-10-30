cmake_minimum_required( VERSION 3.20 )

project(
    teleaudio
    VERSION 0.0.1
    DESCRIPTION "Streaming audio from a distance"
    LANGUAGES CXX
)

# TODO: add clang-format

set( CMAKE_CXX_STANDARD 20 )

add_subdirectory(proto)

add_subdirectory(src)

add_subdirectory(test)
