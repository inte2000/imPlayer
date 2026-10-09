add_library(wavpack_decoder SHARED PlugMain.cpp WavpackFunc.cpp WavpackPlayCtrl.cpp)
iplayer_configure_decoder(wavpack_decoder wavpack_decoder.def)

find_package(wavpack CONFIG REQUIRED)
target_link_libraries(wavpack_decoder PRIVATE WavPack::WavPack)
