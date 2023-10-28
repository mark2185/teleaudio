include_guard()

if ( MSVC )
    add_compile_options( "/W4" "$<$<CONFIG:RELEASE>:/O2>" )
    add_definitions( "-D_CRT_SECURE_NO_WARNINGS" ) # it complains about fopen, I don't really feel like using fopen_s
else()
    # TODO: -fsanitize causes an ABI break with gRPC?
    add_compile_options("-Wall" "-Wextra" "-Werror" "-Wno-ignored-attributes" "-Wconversion" "$<$<CONFIG:RELEASE>:-O3>")
endif()

find_package( spdlog REQUIRED )

set( SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/audio_server.cpp
    ${PROJECT_SOURCE_DIR}/include/audio_server.hpp
    ${CMAKE_CURRENT_LIST_DIR}/audio_client.cpp
    ${PROJECT_SOURCE_DIR}/include/audio_client.hpp
    ${CMAKE_CURRENT_LIST_DIR}/wav.cpp
    ${PROJECT_SOURCE_DIR}/include/wav.hpp
)

set( target libteleaudio )

add_library( ${target} ${SOURCES} )

# gRPC depends on systemd if it can find it, can't turn it off
# Source: https://github.com/grpc/grpc/pull/34384
# that means we can't link statically, since systemd cannot be linked statically
find_library( SYSTEMD_LIB systemd )

if ( SYSTEMD_LIB )
    set( target_libraries "systemd" )
endif()

if ( WIN32 )
    set( target_libraries "winmm" ) # for playing audio
endif()

target_link_libraries( ${target} PUBLIC spdlog::spdlog proto ${target_libraries} )

set_target_properties(
    ${target}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    INCLUDE_DIRECTORIES      "${PROJECT_SOURCE_DIR}/include"
)

add_executable( teleaudio ${CMAKE_CURRENT_LIST_DIR}/main.cpp )
target_link_libraries( teleaudio PRIVATE libteleaudio )

set_target_properties(
    teleaudio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/include"
)
