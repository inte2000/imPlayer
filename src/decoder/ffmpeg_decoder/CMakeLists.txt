add_library(ffmpeg_decoder SHARED
    PlugMain.cpp FfmpegFunc.cpp FfmpegPlayCtrl.cpp PluginConfigFile.cpp
)
iplayer_configure_decoder(ffmpeg_decoder ffmpeg_decoder.def)

find_package(FFMPEG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
target_include_directories(ffmpeg_decoder PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg_decoder PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_decoder PRIVATE
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json
    ftxui::dom ftxui::screen ftxui::component
)
