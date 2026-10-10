find_package(FFMPEG REQUIRED)
find_package(LibSoX REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc i18n data)
find_package(ftxui CONFIG REQUIRED)
find_package(FFTW3 CONFIG REQUIRED)

# Shared implementation objects: main.cpp belongs only to the executable.
add_library(iplayer_objects OBJECT
    source/WavDecoder.cpp
    source/AudioDecoderMap.cpp
    source/PluginConfig.cpp
    source/PluginsMgmt.cpp
    source/DecoderDllWrapper.cpp
    source/PluginDecoder.cpp
    source/DecoderFactory.cpp
    source/DecoderParameters.cpp
    source/DecoderParametersMgmt.cpp
    source/EncoderFactory.cpp
    source/WavEncoder.cpp
    source/ArchivePlaylistBuilder.cpp
    source/SystemFunc.cpp
    source/AudioTarget.cpp
    source/AudioSource.cpp
    source/Music.cpp
    source/PlayList.cpp
    source/PlayListFile.cpp
    source/PlaySequence.cpp
    source/DeviceInfoWin.cpp
    source/AudioDeviceMgmt.cpp
    source/WasapiAudioDevice.cpp
    source/DsAudioDevice.cpp
    source/SpeakerConfig.cpp
    source/GlobalConfig.cpp
    source/Playback.cpp
    source/SpectrumAnalyzer.cpp
    source/SpectrumDisplay.cpp
    source/PlayInterface.cpp
    source/TUIPlayerUI.cpp
    source/SoxEffects.cpp
    source/SoxInputEffect.cpp
    source/SoxOutputEffect.cpp
)
target_include_directories(iplayer_objects PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${THIRDLIB_ROOT}/dr_libs/include"
)
target_include_directories(iplayer_objects PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(iplayer_objects PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(iplayer_objects PUBLIC
    core
    ${FFMPEG_LIBRARIES}
    LibSoX::LibSoX
    nlohmann_json::nlohmann_json
    ICU::uc ICU::i18n ICU::data
    ftxui::dom ftxui::screen ftxui::component
    FFTW3::fftw3
    ole32 uuid winmm dsound avrt
)

add_executable(imPlayer source/main.cpp)
target_link_libraries(imPlayer PRIVATE iplayer_objects)
target_compile_definitions(imPlayer PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
add_dependencies(imPlayer ${IPLAYER_DECODER_TARGETS})

# Seed runtime configuration without overwriting settings edited by the user.
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CopyRuntimeConfig-$<CONFIG>.cmake" CONTENT
"file(MAKE_DIRECTORY \"$<TARGET_FILE_DIR:imPlayer>/config\")
if(NOT EXISTS \"$<TARGET_FILE_DIR:imPlayer>/config/plugin.config\")
    configure_file(\"${PROJECT_SOURCE_DIR}/config/plugin.config\" \"$<TARGET_FILE_DIR:imPlayer>/config/plugin.config\" COPYONLY)
endif()
")
add_custom_command(TARGET imPlayer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/CopyRuntimeConfig-$<CONFIG>.cmake"
    VERBATIM
)
