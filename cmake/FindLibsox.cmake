
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIBSOX_ARCH x64)
else()
    set(LIBSOX_ARCH x86)
endif()

find_path(LIBSOX_INCLUDE_DIR
    NAMES sox.h
    PATHS 
        ${THIRDLIB_ROOT}/sox-14.4.2/src
        ${THIRDLIB_ROOT}/sox/src
    NO_DEFAULT_PATH
)

find_library(LIBSOX_LIBRARY_DIR
    NAMES LibSoX
    PATHS
        ${THIRDLIB_ROOT}/sox-14.4.2/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
        ${THIRDLIB_ROOT}/sox/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
    NO_DEFAULT_PATH
)

find_library(LIBSOX_GSM_LIBRARY_DIR
    NAMES LibGsm
    PATHS
        ${THIRDLIB_ROOT}/sox-14.4.2/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
        ${THIRDLIB_ROOT}/sox/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
    NO_DEFAULT_PATH
)

find_library(LIBSOX_LPC10_LIBRARY_DIR
    NAMES LibLpc10
    PATHS
        ${THIRDLIB_ROOT}/sox-14.4.2/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
        ${THIRDLIB_ROOT}/sox/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
    NO_DEFAULT_PATH
)

find_library(LIBSOX_SPEEX_LIBRARY_DIR
    NAMES LibSpeex
    PATHS
        ${THIRDLIB_ROOT}/sox-14.4.2/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
        ${THIRDLIB_ROOT}/sox/msvc10/${LIBSOX_ARCH}/${CMAKE_BUILD_TYPE}
    NO_DEFAULT_PATH
)



include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibSoX
    REQUIRED_VARS 
      LIBSOX_INCLUDE_DIR 
      LIBSOX_LIBRARY_DIR 
      LIBSOX_GSM_LIBRARY_DIR 
      LIBSOX_LPC10_LIBRARY_DIR 
      LIBSOX_SPEEX_LIBRARY_DIR
)

if(LibSoX_FOUND)
    add_library(LibSoX::LibSoX UNKNOWN IMPORTED)
    set_target_properties(LibSoX::LibSoX PROPERTIES
        IMPORTED_LOCATION "${LIBSOX_LIBRARY_DIR}"
        INTERFACE_LINK_LIBRARIES "${LIBSOX_GSM_LIBRARY_DIR};${LIBSOX_LPC10_LIBRARY_DIR};${LIBSOX_SPEEX_LIBRARY_DIR}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSOX_INCLUDE_DIR}"
    )

# set_target_properties(Xxx::Xxx PROPERTIES
#     INTERFACE_LINK_LIBRARIES
#        $<$<CONFIG:Debug>:
#            ${XXX_CORE_LIB_DEBUG};
#            ${XXX_CODEC_LIB_DEBUG};
#            ${XXX_UTILS_LIB_DEBUG}
#        >
#        $<$<CONFIG:Release>:
#            ${XXX_CORE_LIB_RELEASE};
#            ${XXX_CODEC_LIB_RELEASE};
#            ${XXX_UTILS_LIB_RELEASE}
#        >
#)

endif()



