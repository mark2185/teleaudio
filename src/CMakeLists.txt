set( target teleaudio )

set( SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/audio_server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set( PROTO_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/communication.proto)

find_package( gRPC     REQUIRED )
find_package( Protobuf REQUIRED )
find_package( spdlog   REQUIRED )


protobuf_generate_cpp( PROTO_SRCS PROTO_HDRS ${PROTO_SOURCES} )

message(WARNING "Srcs: ${PROTO_SRCS}, hdrs: ${PROTO_HDRS}" )
message(WARNING "Protobuf libraries: ${Protobuf_PROTOC_LIBRARIES}" )
# target_include_directories( ${target} PRIVATE )

add_executable( ${target} ${SOURCES} ${PROTO_SRCS} ${PROTO_HDRS} )
target_link_libraries( ${target} spdlog::spdlog gRPC::grpc protobuf::libprotobuf )

set_target_properties(
    teleaudio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/src"
)
