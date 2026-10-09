add_library(libflac_decoder SHARED PlugMain.cpp FlacFunc.cpp FlacPlayCtrl.cpp)
iplayer_configure_decoder(libflac_decoder libflac_decoder.def)

find_package(FLAC CONFIG REQUIRED)
target_link_libraries(libflac_decoder PRIVATE FLAC::FLAC)
