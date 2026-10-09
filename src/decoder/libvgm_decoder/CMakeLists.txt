add_library(libvgm_decoder SHARED
    PlugMain.cpp LibvgmFunc.cpp LibVgmPlayCtrl.cpp PluginConfigFile.cpp
)
iplayer_configure_decoder(libvgm_decoder libvgm_decoder.def)

find_package(Libvgm REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
target_link_libraries(libvgm_decoder PRIVATE
    Libvgm::Libvgm nlohmann_json::nlohmann_json
    ftxui::dom ftxui::screen ftxui::component
)
