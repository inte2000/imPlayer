find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)
find_path(LIBGME_INCLUDE_DIR NAMES gme/gme.h REQUIRED)
find_library(LIBGME_LIBRARY NAMES gme REQUIRED)
find_package(SndFile CONFIG REQUIRED)

add_executable(imPlayerTests
    main.cpp
    CoreTests.cpp
    CDSectorsStreamTests.cpp
    ArchiveTests.cpp
    FileStreamMateSourceTests.cpp
    NetStreamTests.cpp
    PluginParseHelperTests.cpp
    SyncRingBufferTests.cpp
    MediaTagTests.cpp
    StringExTests.cpp
    UnicodeConvertTests.cpp
    UnicodeNormalizeTests.cpp
    Utf8StringTests.cpp
    AudioDecoderMapTests.cpp
    DecoderTests.cpp
    DecoderFactoryTests.cpp
    FfmpegParserTests.cpp
    GmeParserTests.cpp
    LibsndParserTests.cpp
    PluginConfigTests.cpp
    WavParserTests.cpp
    EncoderTests.cpp
    PlayerTests.cpp
    "${PROJECT_SOURCE_DIR}/src/decoder/ffmpeg_decoder/FfmpegFunc.cpp"
    "${PROJECT_SOURCE_DIR}/src/decoder/gme_decoder/GmeFunc.cpp"
    "${PROJECT_SOURCE_DIR}/src/decoder/libsnd_decoder/LibsndFunc.cpp"
)
target_include_directories(imPlayerTests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/core/include"
    "${PROJECT_SOURCE_DIR}/src/player/include"
    "${PROJECT_SOURCE_DIR}/src/decoder/ffmpeg_decoder"
    "${PROJECT_SOURCE_DIR}/src/decoder/gme_decoder"
    "${PROJECT_SOURCE_DIR}/src/decoder/libsnd_decoder"
    ${LIBGME_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)
target_compile_definitions(imPlayerTests PRIVATE TEST_SOURCE_ROOT="${PROJECT_SOURCE_DIR}")
target_link_libraries(imPlayerTests PRIVATE
    iplayer_objects
    Catch2::Catch2
    nlohmann_json::nlohmann_json
    ${LIBGME_LIBRARY}
)
if(TARGET SndFile::sndfile)
    target_link_libraries(imPlayerTests PRIVATE SndFile::sndfile)
elseif(TARGET sndfile)
    target_link_libraries(imPlayerTests PRIVATE sndfile)
else()
    message(FATAL_ERROR "SndFile package did not provide a supported target")
endif()

include(Catch)
catch_discover_tests(imPlayerTests WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# The layout integration test exercises the actual host and all plugin artifacts.
add_dependencies(imPlayerTests imPlayer)
add_test(NAME RuntimeLayoutAndPluginLoading
    COMMAND "${CMAKE_COMMAND}"
        "-DPLAYER=$<TARGET_FILE:imPlayer>"
        "-DCORE=$<TARGET_FILE:core>"
        "-DEXPECTED_BIN=$<IF:$<CONFIG:Debug>,${IPLAYER_DEBUG_OUTPUT_DIR},${IPLAYER_RELEASE_OUTPUT_DIR}>"
        "-DPLUGIN_TARGETS=${IPLAYER_DECODER_TARGETS}"
        "-DSOURCE_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RuntimeLayoutTests.cmake"
)
