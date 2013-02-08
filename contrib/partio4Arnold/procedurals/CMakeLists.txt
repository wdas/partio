##### THIS WILL NEED SOME HELP WITH  NON-Linux compiles

## CMake compatibility issues: don't modify this, please!
CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )

if(COMMAND cmake_policy)
     cmake_policy(SET CMP0003 NEW)
	 cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
endif(COMMAND cmake_policy)

IF ( UNIX AND NOT APPLE )
	ADD_DEFINITIONS(-DLINUX)
ENDIF()
if( APPLE )
	ADD_DEFINITIONS(-DMAC_PLUGIN)
	ADD_DEFINITIONS(-DOSMac_)
	ADD_DEFINITIONS(-DOSMac_MachO)
ENDIF()

SET(CMAKE_CXX_WARNING_LEVEL 4)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
## Use verbose make file
SET ( CMAKE_VERBOSE_MAKEFILE TRUE )

set (PLUGIN_NAME "partioGenerator")
set (CMAKE_PROJECT_NAME "partioGenerator")

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
find_package(MtoA)

## if building from the default partio contrib dir you can leave this alone
SET (PARTIO_INCLUDE "../../../include" )
SET (PARTIO_LIBDIR   "../../../lib")

set (STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

include_directories (${STD_INCLUDES} ${partio4ArnoldINCLUDES} ${ARNOLD_INCLUDE_DIR} ${MTOA_INCLUDE_DIR} ${PARTIO_INCLUDE})
link_directories  ( ${STD_LIB} ${MtoA_mtoa_api_LIBRARY} ${ARNOLD_LIBRARY}  ${PARTIO_LIBDIR} ${ZLIB_LIBRARY_DIR})

add_definitions(-D_BOOL)
add_definitions(-DREQUIRE_IOSTREAM)
if (!WIN32)
	add_definitions(-fPIC)
	add_definitions(-O3)
	add_definitions(-pthread)
endif()


FILE(GLOB partio4ArnoldCPP "*.cpp")
FILE(GLOB partio4ArnoldINCLUDES "*.h")

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
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ./build)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES(${PLUGIN_NAME} ${ZLIB} ${PARTIO_LIB} ${ARNOLD_LIBRARY} ${MtoA_mtoa_api_LIBRARY} )

#INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/ DESTINATION contrib/partio4 )

