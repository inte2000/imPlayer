cmake_minimum_required(VERSION 3.21)

project(libogg_decoder)

add_library(libogg_decoder SHARED
    PlugMain.cpp
    OggFunc.cpp
    OggPlayCtrl.cpp
)

target_include_directories(libogg_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)

if(MSVC)
    target_link_options(libogg_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/libogg_decoder.def")
endif()

set_target_properties(libogg_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to libogg_decoder")
    endif()
endif()

find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
target_link_libraries(libogg_decoder PRIVATE
    core
    Ogg::ogg
    Vorbis::vorbisfile
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
