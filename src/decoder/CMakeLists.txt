include("${PROJECT_SOURCE_DIR}/cmake/DecoderPlugin.cmake")

set(IPLAYER_DECODER_TARGETS
    libsnd_decoder
    libvgm_decoder
    gme_decoder
    mpg123_decoder
    libflac_decoder
    libogg_decoder
    wavpack_decoder
    ffmpeg_decoder
    radio_decoder
    radio_decoder2
    cdtrack_decoder
)
foreach(plugin IN LISTS IPLAYER_DECODER_TARGETS)
    add_subdirectory(${plugin})
endforeach()
set(IPLAYER_DECODER_TARGETS "${IPLAYER_DECODER_TARGETS}" PARENT_SCOPE)
