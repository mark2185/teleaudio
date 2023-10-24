set( target teleaudio )

find_package( gRPC     REQUIRED )
find_package( Protobuf CONFIG REQUIRED )
find_package( spdlog   REQUIRED )


set( SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/audio_server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set( PROTO_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/communication.proto)

add_library( myproto ${PROTO_SOURCES} )
target_link_libraries( myproto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
# message(WARNING "Location to the grpc plugin is ${grpc_cpp_plugin_location}" )
target_include_directories(myproto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# message(WARNING "Protoc: ${_gRPC_PROTOBUF_PROTOC_EXECUTABLE}")
# protobuf_generate(
    # TARGET myproto
    # LANGUAGE grpc
    # GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    # PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
    # PLUGIN_OPTIONS
# )
protobuf_generate(TARGET myproto LANGUAGE cpp)
set(_GRPC_PLUGIN "protoc-gen-grpc=${_GRPC_CPP_PLUGIN}")
protobuf_generate(TARGET myproto PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}" LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc)
# add_custom_command(
    # COMMAND ${Protobuf_jkk}
    # ARGS --grpc_out=generate_mock_code=true:${_gRPC_PROTO_GENS_DIR}
         # --cpp_out=${_gRPC_PROTO_GENS_DIR}
         # --plugin=protoc-gen-grpc=${_gRPC_CPP_PLUGIN}
         # ${_protobuf_include_path}
         # ${REL_FIL}
# )

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
    # message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

# message(WARNING "Srcs: ${PROTO_SRCS}, hdrs: ${PROTO_HDRS}" )
# message(WARNING "Protobuf libraries: ${Protobuf_PROTOC_LIBRARIES}" )
# target_include_directories( ${target} PRIVATE )

add_executable( ${target} ${SOURCES} )
target_link_libraries( ${target} spdlog::spdlog gRPC::grpc gRPC::grpc++ myproto )

set_target_properties(
    teleaudio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/src"
)
