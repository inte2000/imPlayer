add_library(radio_decoder SHARED
    PlugMain.cpp FfmpegFunc.cpp FfmpegPlayCtrl.cpp PluginConfigFile.cpp
)
iplayer_configure_decoder(radio_decoder radio_decoder.def)
target_include_directories(radio_decoder PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(radio_decoder PRIVATE FFMPEG_DECODER_CONFIG_FILE_NAME="radio_decoder.config")

find_package(FFMPEG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
target_include_directories(radio_decoder PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(radio_decoder PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(radio_decoder PRIVATE
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    ftxui::dom ftxui::screen ftxui::component
)
