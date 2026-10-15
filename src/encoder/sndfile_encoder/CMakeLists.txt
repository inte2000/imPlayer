add_library(sndfile_encoder SHARED PluginMain.cpp PluginMain.h EncodeControl.cpp EncodeControl.h sndfile_encoder.def)
target_link_libraries(sndfile_encoder PRIVATE core)
find_package(SndFile CONFIG REQUIRED)
if(TARGET SndFile::sndfile)
    target_link_libraries(sndfile_encoder PRIVATE SndFile::sndfile)
elseif(TARGET sndfile)
    target_link_libraries(sndfile_encoder PRIVATE sndfile)
else()
    message(FATAL_ERROR "SndFile package did not provide a supported target")
endif()
set_target_properties(sndfile_encoder PROPERTIES
    PREFIX ""
    SUFFIX ".opeplug"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${IPLAYER_DEBUG_OUTPUT_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${IPLAYER_DEBUG_OUTPUT_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${IPLAYER_RELEASE_OUTPUT_DIR}/plugins"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${IPLAYER_RELEASE_OUTPUT_DIR}/plugins"
)
