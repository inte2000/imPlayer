cmake_minimum_required(VERSION 3.21)

project(player)

add_library(player STATIC
    SystemFunc.cpp
    AudioTarget.cpp
    AudioSource.cpp
    Music.cpp
    PlayList.cpp
    PlayListFile.cpp
    PlaySequence.cpp
    DeviceInfoWin.cpp
    AudioDeviceMgmt.cpp
    WasapiAudioDevice.cpp
    DsAudioDevice.cpp
    SpeakerConfig.cpp
    GlobalConfig.cpp
    Playback.cpp
    PlayInterface.cpp
    TUIPlayerUI.cpp
    SoxEffects.cpp
    SoxInputEffect.cpp
    SoxOutputEffect.cpp
)

target_include_directories(player PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(player PRIVATE ${CMAKE_SOURCE_DIR}/include/core)
target_include_directories(player PRIVATE ${CMAKE_SOURCE_DIR}/include/decoder)
target_include_directories(player PRIVATE ${CMAKE_SOURCE_DIR}/include/player)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


find_package(LibSoX REQUIRED)
  target_link_libraries(player PRIVATE LibSoX::LibSoX)

find_package(nlohmann_json CONFIG REQUIRED)
  target_link_libraries(player PRIVATE nlohmann_json::nlohmann_json)

find_package(ICU REQUIRED COMPONENTS uc i18n data)
  target_link_libraries(player PRIVATE ICU::uc ICU::i18n ICU::data)

find_package(ftxui CONFIG REQUIRED)
  target_link_libraries(player PRIVATE ftxui::dom ftxui::screen ftxui::component)