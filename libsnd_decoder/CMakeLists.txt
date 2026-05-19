cmake_minimum_required(VERSION 3.21)

project(libsnd_decoder)

add_library(libsnd_decoder SHARED
    PlugMain.cpp
    LibsndFunc.cpp
    LibSndPlayCtrl.cpp
)

target_include_directories(libsnd_decoder PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)
if (MSVC)
    target_link_options(libsnd_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/libsnd_decoder.def")
endif()

set_target_properties(libsnd_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to libsnd_decoder")
    endif()
endif()

find_package(SndFile CONFIG REQUIRED)
if(TARGET SndFile::sndfile)
    target_link_libraries(libsnd_decoder PRIVATE core SndFile::sndfile)
elseif(TARGET sndfile)
    target_link_libraries(libsnd_decoder PRIVATE core sndfile)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
