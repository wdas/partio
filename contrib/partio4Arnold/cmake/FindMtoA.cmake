#-*-cmake-*-
#
# yue.nicholas@gmail.com
#
# This auxiliary CMake file helps in find the MtoA headers and libraries
#
# MTOA_FOUND            set if MtoA is found.
# MTOA_INCLUDE_DIR      MtoA's include directory
# MtoA_mtoa_api_LIBRARY Full path location of libmtoa_api

FIND_PACKAGE ( PackageHandleStandardArgs )

##
## Obtain MtoA install location
##
FIND_PATH( MTOA_LOCATION include/render/AOV.h
  "$ENV{MTOA_ROOT}"
  NO_DEFAULT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS ( MtoA
  REQUIRED_VARS MTOA_LOCATION
  )

IF (MTOA_FOUND)

  SET ( ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  IF (WIN32)
  ELSEIF (APPLE)
	SET( MTOA_LIBRARY_DIR "${MTOA_LOCATION}/bin" CACHE_STRING "MtoA library path" )
    FIND_LIBRARY ( MtoA_mtoa_api_LIBRARY  mtoa_api  ${MTOA_LIBRARY_DIR} )
  ELSE (WIN32)
	# Unices
	SET( MTOA_LIBRARY_DIR "${MTOA_LOCATION}/bin" CACHE_STRING "MtoA library path" )
    FIND_LIBRARY ( MtoA_mtoa_api_LIBRARY  mtoa_api  ${MTOA_LIBRARY_DIR} )
  ENDIF (WIN32)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES})
  
  SET( MTOA_INCLUDE_DIR "${MTOA_LOCATION}/include" CACHE STRING "MtoA include path")

  
ENDIF (MTOA_FOUND)
