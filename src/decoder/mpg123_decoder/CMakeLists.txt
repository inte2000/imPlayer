add_library(mpg123_decoder SHARED
    PlugMain.cpp Mpg123Func.cpp Mpg123PlayCtrl.cpp Mp3Handle.cpp
)
iplayer_configure_decoder(mpg123_decoder mpg123_decoder.def)

find_package(mpg123 CONFIG REQUIRED)
target_link_libraries(mpg123_decoder PRIVATE
    MPG123::libmpg123 MPG123::libout123 MPG123::libsyn123
)
