set( target teleaudio )

if ( MSVC )
    add_compile_options( "/W4" "$<$<CONFIG:RELEASE>:/O2>" )
	add_definitions( "-D_CRT_SECURE_NO_WARNINGS" ) # it complains about fopen, I don't really feel like using fopen_s
else()
    # TODO: -fsanitize causes an ABI break with gRPC?
    add_compile_options("-Wall" "-Wextra" "-Werror" "-Wno-ignored-attributes" "-Wconversion" "$<$<CONFIG:RELEASE>:-O3>")
endif()

find_package( gRPC     REQUIRED )
find_package( Protobuf CONFIG REQUIRED )
find_package( spdlog   REQUIRED )

set( SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/audio_server.cpp
    ${PROJECT_SOURCE_DIR}/include/audio_server.hpp
    ${CMAKE_CURRENT_LIST_DIR}/audio_client.cpp
    ${PROJECT_SOURCE_DIR}/include/audio_client.hpp
    ${CMAKE_CURRENT_LIST_DIR}/wav.cpp
    ${PROJECT_SOURCE_DIR}/include/wav.hpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set( PROTO_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/communication.proto)

add_library( myproto STATIC ${PROTO_SOURCES} )
target_link_libraries( myproto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++)

set_target_properties(
    myproto
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

get_target_property( grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION )
target_include_directories(myproto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET myproto LANGUAGE cpp)
protobuf_generate(TARGET myproto PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}" LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc)

add_executable( ${target} ${SOURCES} )

# gRPC depends on systemd if it can find it, can't turn it off
# Source: https://github.com/grpc/grpc/pull/34384
# that means we can't build statically if systemd is found, since it cannot be linked statically
find_library( SYSTEMD_LIB systemd )

if ( SYSTEMD_LIB )
    set( target_libraries "${target_libraries} systemd" )
endif()

if ( WIN32 )
    set( target_libraries "${target_libraries} winmm" ) # for playing audio
endif()

target_link_libraries( ${target} PRIVATE spdlog::spdlog myproto ${target_libraries} )

set_target_properties(
    teleaudio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INCLUDE_DIRECTORIES      "${PROJECT_SOURCE_DIR}/include"
)
