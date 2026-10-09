function(iplayer_configure_decoder target export_file)
    target_link_libraries(${target} PRIVATE core)
    target_link_options(${target} PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/${export_file}")
    set_target_properties(${target} PROPERTIES
        SUFFIX ".ipdplus"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${IPLAYER_DEBUG_OUTPUT_DIR}/plugins"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${IPLAYER_DEBUG_OUTPUT_DIR}/plugins"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${IPLAYER_RELEASE_OUTPUT_DIR}/plugins"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${IPLAYER_RELEASE_OUTPUT_DIR}/plugins"
    )
endfunction()
