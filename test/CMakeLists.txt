include_guard()

enable_testing()

set( SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/test_main.cpp
)

set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/storage")
get_filename_component(RESOURCES_ABS_PATH ${RESOURCES_PATH} ABSOLUTE)

add_executable( TeleaudioTest ${SOURCES} )

find_package( GTest REQUIRED )

target_link_libraries( TeleaudioTest PRIVATE libteleaudio GTest::gtest )

set_target_properties(
    TeleaudioTest
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/include"
)

add_test(
    NAME    TeleaudioTest
    COMMAND TeleaudioTest "${CMAKE_CURRENT_SOURCE_DIR}/storage"
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
)
