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

get_target_property( grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION )
target_include_directories(myproto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(TARGET myproto LANGUAGE cpp)
protobuf_generate(TARGET myproto PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}" LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

add_executable( ${target} ${SOURCES} )

# TODO: don't link systemd on windows
target_link_libraries( ${target} PUBLIC spdlog::spdlog gRPC::grpc gRPC::grpc++ myproto systemd )

set_target_properties(
    teleaudio
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/src"
)
