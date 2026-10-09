add_library(core STATIC
    source/AudioInfo.cpp
    source/AudioCD.cpp
    source/StringEx.cpp
    source/ArchivePackage.cpp
    source/ArchiveFile.cpp
    source/ArchiveFileStream.cpp
    source/UnicodeNormalize.cpp
    source/UnicodeConvert.cpp
    source/FileStream.cpp
    source/DummyNameStream.cpp
    source/LibCurlInit.cpp
    source/NetStream.cpp
    source/SyncRingBuffer.cpp
    source/CDSectorsStream.cpp
    source/MemBufStream.cpp
    source/ConvertFormat.cpp
    source/AudioConvert.cpp
    source/MediaTag.cpp
    source/MediaTagNames.cpp
    source/Utf8String.cpp
    source/StdFileSystem.cpp
    source/RiffUtils.cpp
)
target_include_directories(core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(core PUBLIC cxx_std_20)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc i18n data)
find_package(FFMPEG REQUIRED)
find_package(libcdio REQUIRED)
find_package(CURL REQUIRED)
find_package(LibArchive REQUIRED)

target_include_directories(core PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(core PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(core PRIVATE
    nlohmann_json::nlohmann_json
    ICU::uc ICU::i18n ICU::data
    ${FFMPEG_LIBRARIES}
    libcdio::libcdio
    CURL::libcurl
    LibArchive::LibArchive
)
