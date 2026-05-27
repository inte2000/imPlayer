cmake_minimum_required(VERSION 3.21)

project(libflac_decoder)

add_library(libflac_decoder SHARED
    PlugMain.cpp
    FlacFunc.cpp
    FlacPlayCtrl.cpp
)

target_include_directories(libflac_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)

if(MSVC)
    target_link_options(libflac_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/libflac_decoder.def")
endif()

set_target_properties(libflac_decoder PROPERTIES
    SUFFIX ".ipdplus"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/plugins"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/plugins"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/plugins"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

if(MSVC)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/msvc_options.cmake")
        include(msvc_options)
        message(STATUS "MSVC options applied to libflac_decoder")
    endif()
endif()

find_package(FLAC CONFIG REQUIRED)
target_link_libraries(libflac_decoder PRIVATE
    core
    FLAC::FLAC
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
