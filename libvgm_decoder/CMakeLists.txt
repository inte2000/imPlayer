cmake_minimum_required(VERSION 3.21)

project(libvgm_decoder)

add_library(libvgm_decoder SHARED
    PlugMain.cpp
    LibvgmFunc.cpp
    LibVgmPlayCtrl.cpp
    PluginConfigFile.cpp
)

target_include_directories(libvgm_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)

if(MSVC)
    target_link_options(libvgm_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/libvgm_decoder.def")
endif()

set_target_properties(libvgm_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to libvgm_decoder")
    endif()
endif()

find_package(Libvgm REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
target_link_libraries(libvgm_decoder PRIVATE core Libvgm::Libvgm nlohmann_json::nlohmann_json ftxui::dom ftxui::screen ftxui::component)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
