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

set (PLUGIN_NAME "partio4Maya")
set (CMAKE_PROJECT_NAME "partio4Maya")

if(APPLE)
	set (PLUGIN_SUFFIX ".bundle")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".bundle")
	set (SUFFIX ".bundle")
ENDIF()

if(WIN32)
	set (PLUGIN_SUFFIX ".mll")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".mll")
	set (SUFFIX ".mll")
ENDIF()

if (UNIX)
	set (PLUGIN_SUFFIX ".so")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".so")
	set (SUFFIX ".so")
ENDIF()
	
find_package(MAYA)

## if building from the default partio contrib dir you can leave this alone
SET (PARTIO_INCLUDE "../../include" )
SET (PARTIO_LIBDIR   "../../lib")

set (STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

include_directories (${STD_INCLUDES} ${partio4MayaINCLUDES} ${MAYA_INCLUDE_DIRS} ${PARTIO_INCLUDE})
link_directories  ( ${STD_LIB} ${MAYA_LIBRARY_DIR} ${PARTIO_LIBDIR} ${ZLIB_LIBRARY_DIR})

add_definitions(-D_BOOL)
add_definitions(-DREQUIRE_IOSTREAM)
if (!WIN32)
	add_definitions(-fPIC)
	add_definitions(-O3)
	add_definitions(-pthread)
endif()


FILE(GLOB partio4MayaCPP "*.cpp")
FILE(GLOB partio4MayaINCLUDES "*.h")

SET(TARGET_H ${partio4MayaINCLUDES})
SET(TARGET_SRC ${partio4MayaCPP})

if(APPLE)
	SET(OPENGL_gl_LIBRARY "-framework OpenGL")
	SET(OPENGL_glu_LIBRARY "-framework GLUT")
	SET(ZLIB "libzlib.a")
elseif(WIN32)

	SET(OPENGL_gl_LIBRARY "openGL32")
	SET(OPENGL_glu_LIBRARY "glu32")
	SET(ZLIB "zlib.lib")
else()
	SET(OPENGL_gl_LIBRARY "GL")
	SET(OPENGL_glu_LIBRARY "GLU")
	SET(ZLIB "z")
endif()

SET(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)

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

## need these to build for windows or else the plugin won't load
IF(WIN32)
	set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin")
ENDIF()

TARGET_LINK_LIBRARIES(${PLUGIN_NAME} ${ZLIB} ${PARTIO_LIB} ${MAYA_LIBS} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})

