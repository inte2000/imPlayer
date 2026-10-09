add_library(libogg_decoder SHARED PlugMain.cpp OggFunc.cpp OggPlayCtrl.cpp)
iplayer_configure_decoder(libogg_decoder libogg_decoder.def)

find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
target_link_libraries(libogg_decoder PRIVATE Ogg::ogg Vorbis::vorbisfile)
