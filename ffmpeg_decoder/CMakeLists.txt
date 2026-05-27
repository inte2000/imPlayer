cmake_minimum_required(VERSION 3.21)

project(ffmpeg_decoder)

add_library(ffmpeg_decoder SHARED
    PlugMain.cpp
    FfmpegFunc.cpp
    FfmpegPlayCtrl.cpp
    PluginConfigFile.cpp
)

target_include_directories(ffmpeg_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)

if(MSVC)
    target_link_options(ffmpeg_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_decoder.def")
endif()

set_target_properties(ffmpeg_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to ffmpeg_decoder")
    endif()
endif()

find_package(FFMPEG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)

target_include_directories(ffmpeg_decoder PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg_decoder PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_decoder PRIVATE
    core
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    ftxui::dom
    ftxui::screen
    ftxui::component
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
