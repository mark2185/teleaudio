set( target teleaudio )

set( SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/audio_server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set( PROTO_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/communication.proto)

find_package( gRPC   REQUIRED )
find_package( spdlog REQUIRED )

add_executable( ${target} ${SOURCES} )

protobuf_generate_cpp( PROTO_SRCS PROTO_HDRS ${PROTO_SOURCES} )

# target_include_directories( ${target} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} )

target_link_libraries( teleaudio spdlog::spdlog gRPC::grpc )

set_target_properties(
    teleaudio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
