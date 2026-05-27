cmake_minimum_required(VERSION 3.21)

project(mpg123_decoder)

add_library(mpg123_decoder SHARED
    PlugMain.cpp
    Mpg123Func.cpp
    Mpg123PlayCtrl.cpp
    Mp3Handle.cpp
)

target_include_directories(mpg123_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)

if(MSVC)
    target_link_options(mpg123_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/mpg123_decoder.def")
endif()

set_target_properties(mpg123_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to mpg123_decoder")
    endif()
endif()

find_package(mpg123 CONFIG REQUIRED)
target_link_libraries(mpg123_decoder PRIVATE
    core
    MPG123::libmpg123
    MPG123::libout123
    MPG123::libsyn123
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
