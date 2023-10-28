include_guard()

set( target proto )

find_package( Protobuf CONFIG REQUIRED )
find_package( gRPC            REQUIRED )

set( PROTO_DIR
    ${PROJECT_SOURCE_DIR}/proto
)

set( PROTO_SOURCES
    ${PROTO_DIR}/communication.proto
)

add_library( proto STATIC ${PROTO_SOURCES} )
target_link_libraries( proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++)

set_target_properties(
    proto
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

get_target_property( grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION )
target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(
    TARGET proto
    LANGUAGE cpp
    IMPORT_DIRS ${PROTO_DIR}
)
protobuf_generate(
    TARGET proto
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    IMPORT_DIRS ${PROTO_DIR}
)
