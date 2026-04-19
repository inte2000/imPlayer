cmake_minimum_required(VERSION 3.21)

project(decoder)

add_library(decoder STATIC
    NativeDecoder.cpp
    Mp3Handle.cpp
    Mp3Decoder.cpp
    DecoderFactory.cpp
    DecoderParameters.cpp
    DecoderParametersMgmt.cpp
)

target_include_directories(decoder PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(decoder PRIVATE ${CMAKE_SOURCE_DIR}/include/core)
target_include_directories(decoder PRIVATE ${CMAKE_SOURCE_DIR}/include/decoder)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(MSVC)
    include(msvc_options)
    message(STATUS "MSVC options applied")
endif()

find_package(nlohmann_json CONFIG REQUIRED)
  target_link_libraries(decoder PRIVATE nlohmann_json::nlohmann_json)

find_package(SndFile CONFIG REQUIRED)
  target_link_libraries(decoder PRIVATE SndFile::sndfile)

find_package(mpg123 CONFIG REQUIRED)
  target_link_libraries(decoder PRIVATE MPG123::libmpg123 MPG123::libout123 MPG123::libsyn123)

find_package(FFMPEG REQUIRED)
  target_include_directories(decoder PRIVATE ${FFMPEG_INCLUDE_DIRS})
  target_link_directories(decoder PRIVATE ${FFMPEG_LIBRARY_DIRS})
  target_link_libraries(decoder PRIVATE ${FFMPEG_LIBRARIES})




