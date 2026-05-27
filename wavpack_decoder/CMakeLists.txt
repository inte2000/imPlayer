cmake_minimum_required(VERSION 3.21)

project(wavpack_decoder)

add_library(wavpack_decoder SHARED
    PlugMain.cpp
    WavpackFunc.cpp
    WavpackPlayCtrl.cpp
)

target_include_directories(wavpack_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)

if(MSVC)
    target_link_options(wavpack_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/wavpack_decoder.def")
endif()

set_target_properties(wavpack_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to wavpack_decoder")
    endif()
endif()

find_package(wavpack CONFIG REQUIRED)
target_link_libraries(wavpack_decoder PRIVATE
    core
    WavPack::WavPack
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
