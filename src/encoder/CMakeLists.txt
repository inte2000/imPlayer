cmake_minimum_required(VERSION 3.21)

project(encoder)

add_library(encoder STATIC
    EncoderFactory.cpp
    WavEncoder.cpp
)

target_include_directories(encoder PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(encoder PRIVATE ${CMAKE_SOURCE_DIR}/include/core)
target_include_directories(encoder PRIVATE ${CMAKE_SOURCE_DIR}/include/encoder)
target_include_directories(encoder PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/dr_libs/include)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(MSVC)
    include(msvc_options)
    message(STATUS "MSVC options applied")
endif()

target_link_libraries(encoder PRIVATE core)
