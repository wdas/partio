#-*-cmake-*-
#
# yue.nicholas@gmail.com
#
# This auxiliary CMake file helps in find the MtoA headers and libraries
#
# MTOA_FOUND            set if MtoA is found.
# MTOA_INCLUDE_DIR      MtoA's include directory
# MTOA_mtoa_api_LIBRARY Full path location of libmtoa_api

find_package(PackageHandleStandardArgs)

##
## Obtain MtoA install location
##
find_path(MTOA_LOCATION include/render/AOV.h
    "$ENV{MTOA_ROOT}"
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH)

find_package_handle_standard_args(MtoA
    REQUIRED_VARS MTOA_LOCATION)

if(MTOA_FOUND)
    set(ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if(WIN32)
    elseif(APPLE)
        set(MTOA_LIBRARY_DIR "${MTOA_LOCATION}/bin"
            CACHE STRING "MtoA library path")
        find_library(MTOA_mtoa_api_LIBRARY mtoa_api ${MTOA_LIBRARY_DIR})
    else() # Linux
        set(MTOA_LIBRARY_DIR "${MTOA_LOCATION}/bin"
            CACHE STRING "MtoA library path")
        find_library(MTOA_mtoa_api_LIBRARY  mtoa_api  ${MTOA_LIBRARY_DIR})
    endif()
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES})
    set(MTOA_INCLUDE_DIR "${MTOA_LOCATION}/include"
        CACHE STRING "MtoA include path")
        message(STATUS "MTOA location: ${MTOA_LOCATION}")
		message(STATUS "MTOA LIB dir:  ${MTOA_mtoa_api_LIBRARY}")
		message(STATUS "MTOA INCLUDE:  ${MTOA_INCLUDE_DIR}")
endif()
