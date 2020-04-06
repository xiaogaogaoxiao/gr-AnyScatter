INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_ANYSCATTER AnyScatter)

FIND_PATH(
    ANYSCATTER_INCLUDE_DIRS
    NAMES AnyScatter/api.h
    HINTS $ENV{ANYSCATTER_DIR}/include
        ${PC_ANYSCATTER_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    ANYSCATTER_LIBRARIES
    NAMES gnuradio-AnyScatter
    HINTS $ENV{ANYSCATTER_DIR}/lib
        ${PC_ANYSCATTER_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/AnyScatterTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ANYSCATTER DEFAULT_MSG ANYSCATTER_LIBRARIES ANYSCATTER_INCLUDE_DIRS)
MARK_AS_ADVANCED(ANYSCATTER_LIBRARIES ANYSCATTER_INCLUDE_DIRS)
