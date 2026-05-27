cmake_minimum_required(VERSION 3.21)

project(gme_decoder)

add_library(gme_decoder SHARED
    PlugMain.cpp
    GmeFunc.cpp
    GmePlayCtrl.cpp
    PluginConfigFile.cpp
)

target_include_directories(gme_decoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/core
)

if(MSVC)
    target_link_options(gme_decoder PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/gme_decoder.def")
endif()

set_target_properties(gme_decoder PROPERTIES
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
        message(STATUS "MSVC options applied to gme_decoder")
    endif()
endif()

find_path(LIBGME_INCLUDE_DIR NAMES gme/gme.h REQUIRED)
find_library(LIBGME_LIBRARY NAMES gme REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)

target_include_directories(gme_decoder PRIVATE ${LIBGME_INCLUDE_DIR})
target_link_libraries(gme_decoder PRIVATE
    core
    ${LIBGME_LIBRARY}
    nlohmann_json::nlohmann_json
    ftxui::dom
    ftxui::screen
    ftxui::component
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
