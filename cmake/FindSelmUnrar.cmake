if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SELMUNRAR_ARCH x64)
else()
    set(SELMUNRAR_ARCH x86)
endif()

find_path(SELMUNRAR_INCLUDE_DIR
    NAMES unarr.h
    PATHS
        ${THIRDLIB_ROOT}/unarr-1.1.1
    NO_DEFAULT_PATH
)

find_library(SELMUNRAR_LIBRARY
    NAMES unarr
    PATHS
        ${THIRDLIB_ROOT}/unarr-1.1.1/build/${CMAKE_BUILD_TYPE}
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SelmUnrar
    REQUIRED_VARS
        SELMUNRAR_INCLUDE_DIR
        SELMUNRAR_LIBRARY
)

if(SelmUnrar_FOUND AND NOT TARGET SelmUnrar::SelmUnrar)
    add_library(SelmUnrar::SelmUnrar UNKNOWN IMPORTED)
    set_target_properties(SelmUnrar::SelmUnrar PROPERTIES
        IMPORTED_LOCATION "${SELMUNRAR_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SELMUNRAR_INCLUDE_DIR}"
    )
endif()
