##### THIS WILL NEED SOME HELP WITH  NON-Linux compiles

## CMake compatibility issues: don't modify this, please!
CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )

if(COMMAND cmake_policy)
     cmake_policy(SET CMP0003 NEW)
	 cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
endif(COMMAND cmake_policy)

SET(CMAKE_CXX_WARNING_LEVEL 4)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)


MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
## Use verbose make file
SET ( CMAKE_VERBOSE_MAKEFILE TRUE )

set (PLUGIN_NAME "partioShaders")
set (CMAKE_PROJECT_NAME "partioShaders")

if(APPLE)
	set (PLUGIN_SUFFIX ".bundle")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".bundle")
	set (SUFFIX ".bundle")
ENDIF()

if(WIN32)
	set (PLUGIN_SUFFIX ".dll")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
	set (SUFFIX ".dll")
ENDIF()

if (UNIX)
	set (PLUGIN_SUFFIX ".so")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".so")
	set (SUFFIX ".so")
ENDIF()

find_package(Arnold)

SET (PARTIO_INCLUDE ${INCLUDE_OUTPUT_PATH} )
SET (PARTIO_LIBDIR  ${LIBRARY_OUTPUT_PATH} )

set (STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

FILE(GLOB partio4ArnoldCPP "*.cpp")
FILE(GLOB partio4ArnoldINCLUDES "*.h")

include_directories (${STD_INCLUDES} ${partio4ArnoldINCLUDES} ${ARNOLD_INCLUDE_DIR}  ${PARTIO_INCLUDE})
link_directories  ( ${STD_LIB}  ${ARNOLD_LIBRARY}  ${PARTIO_LIBDIR} ${ZLIB_LIBRARY_DIR})

SET(TARGET_H ${partio4ArnoldINCLUDES})
SET(TARGET_SRC ${partio4ArnoldCPP})

IF (WIN32)
	SET(PARTIO_LIB  partio)
elseif(APPLE)
	SET(PARTIO_LIB  libpartio.a)
elseif(UNIX)
	SET(PARTIO_LIB  libpartio.a)
ENDIF()


ADD_LIBRARY(${PLUGIN_NAME} SHARED ${TARGET_H} ${TARGET_SRC})
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ./ )
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES(${PLUGIN_NAME} ${ZLIB} ${PARTIO_LIB} ${ARNOLD_LIBRARY} ${MtoA_mtoa_api_LIBRARY} )

SET (PARTIO4ARNOLDSHADE_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/build-${PARTIO_BUILD_PLATFORM}/arnold/mtoa-$ENV{MTOA_VERSION}/${PARTIO_BUILD_PLATFORM}/shaders)

INSTALL( TARGETS ${PLUGIN_NAME} DESTINATION ${PARTIO4ARNOLDSHADE_OUTPUT_PATH} )

FILE(GLOB PARTIO4ARNOLD_SCRIPTS "*.mel")
INSTALL( FILES ${PARTIO4ARNOLD_SCRIPTS} DESTINATION ${PARTIO4ARNOLDSHADE_OUTPUT_PATH})
