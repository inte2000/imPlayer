cmake_minimum_required(VERSION 3.21)
project(iPlayer VERSION 0.1.2)

if (NOT MSVC)
    message(FATAL_ERROR "This project is intended to be built with MSVC")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IPLAYER_ARCH x64)
else()
    set(IPLAYER_ARCH x86)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

set(OUTPUT_ROOT ${CMAKE_BINARY_DIR}/bin)

foreach(cfg Debug Release)
    string(TOUPPER ${cfg} CFG_UPPER)
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} ${OUTPUT_ROOT}/${cfg})
#    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} ${OUTPUT_ROOT}/${cfg})
#    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG_UPPER} ${OUTPUT_ROOT}/${cfg})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} ${OUTPUT_ROOT})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} ${OUTPUT_ROOT})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG_UPPER} ${OUTPUT_ROOT})
endforeach()

include(cmake/msvc_options.cmake)

# thirdparty root
set(THIRDLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")


# file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB SRC_FILES src/*.cpp)

add_subdirectory(src/core)
add_subdirectory(src/decoder)
add_subdirectory(src/player)
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include/core)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include/player)

# add_executable(${PROJECT_NAME}  ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE core decoder player)
target_link_libraries(${PROJECT_NAME} PRIVATE
        ole32
        uuid
        winmm
        dsound
        avrt       # MMCSS（低延迟音频）
)

# find_package(nlohmann_json CONFIG REQUIRED)
#   target_link_libraries(core PRIVATE nlohmann_json::nlohmann_json)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# find_package(libcdio REQUIRED)
#   target_link_libraries(core PRIVATE libcdio::libcdio)

# find_package(nlohmann_json CONFIG REQUIRED)
#   target_link_libraries(core PRIVATE nlohmann_json::nlohmann_json)

 find_package(ICU REQUIRED COMPONENTS uc i18n data)
  target_link_libraries(${PROJECT_NAME} PRIVATE ICU::uc ICU::i18n ICU::data)
  
# find_package(SndFile CONFIG REQUIRED)
#   target_link_libraries(core PRIVATE SndFile::sndfile)

# find_package(FFMPEG REQUIRED)
#   target_include_directories(core PRIVATE ${FFMPEG_INCLUDE_DIRS})
#   target_link_directories(core PRIVATE ${FFMPEG_LIBRARY_DIRS})
#   target_link_libraries(core PRIVATE ${FFMPEG_LIBRARIES})



# FFmpeg（示例：手动引入）
# set(FFMPEG_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/ffmpeg)
# target_include_directories(iPlayer PRIVATE  ${FFMPEG_ROOT}/include)
# target_link_directories(iPlayer PRIVATE  ${FFMPEG_ROOT}/lib)
# target_link_libraries(iPlayer PRIVATE  avcodec  avformat  avutil  swresample)


# Debug 专用定义

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

