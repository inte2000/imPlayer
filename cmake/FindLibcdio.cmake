
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIBCDIO_ARCH x64)
else()
    set(LIBCDIO_ARCH x86)
endif()

find_path(LIBCDIO_INCLUDE_DIR
    NAMES cdio/cdio.h
    PATHS 
        ${THIRDLIB_ROOT}/libcdio-2.2.0/include
        ${THIRDLIB_ROOT}/libcdio/include
    NO_DEFAULT_PATH
)

find_library(LIBCDIO_LIBRARY_DIR
    NAMES libcdio
    PATHS
        ${THIRDLIB_ROOT}/libcdio-2.2.0/${CMAKE_BUILD_TYPE}/${LIBCDIO_ARCH}
        ${THIRDLIB_ROOT}/libcdio/${CMAKE_BUILD_TYPE}/${LIBCDIO_ARCH}
    NO_DEFAULT_PATH
)

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message(STATUS "THIRDLIB_ROOT = ${THIRDLIB_ROOT}")
message(STATUS "LIBCDIO_LIBRARY_DIR = ${LIBCDIO_LIBRARY_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libcdio
    REQUIRED_VARS LIBCDIO_INCLUDE_DIR LIBCDIO_LIBRARY_DIR
)

if(libcdio_FOUND)
    add_library(libcdio::libcdio UNKNOWN IMPORTED)
    set_target_properties(libcdio::libcdio PROPERTIES
        IMPORTED_LOCATION "${LIBCDIO_LIBRARY_DIR}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBCDIO_INCLUDE_DIR}"
    )
endif()

