add_library(gme_decoder SHARED
    PlugMain.cpp GmeFunc.cpp GmePlayCtrl.cpp PluginConfigFile.cpp
)
iplayer_configure_decoder(gme_decoder gme_decoder.def)

find_path(LIBGME_INCLUDE_DIR NAMES gme/gme.h REQUIRED)
find_library(LIBGME_LIBRARY NAMES gme REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ftxui CONFIG REQUIRED)
target_include_directories(gme_decoder PRIVATE ${LIBGME_INCLUDE_DIR})
target_link_libraries(gme_decoder PRIVATE
    ${LIBGME_LIBRARY}
    nlohmann_json::nlohmann_json
    ftxui::dom ftxui::screen ftxui::component
)
