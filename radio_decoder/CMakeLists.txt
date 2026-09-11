cmake_minimum_required(VERSION 3.21)

project(radio_decoder)

add_library(radio_decoder SHARED
    PlugMain.cpp
    FfmpegFunc.cpp
    FfmpegPlayCtrl.cpp
    PluginConfigFile.cpp
)

target_include_directories(radio_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(radio_decoder PRIVATE
    FFMPEG_DECODER_CONFIG_FILE_NAME="radio_decoder.config"
)

if(MSVC)
    target_link_options(radio_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/radio_decoder.def")
endif()

set_target_properties(radio_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to radio_decoder")
    endif()
endif()

find_package(FFMPEG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)

target_include_directories(radio_decoder PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(radio_decoder PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(radio_decoder PRIVATE
    core
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    ftxui::dom
    ftxui::screen
    ftxui::component
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
