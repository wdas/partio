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

SET(CMAKE_CXX_WARNING_LEVEL 4)


MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
## Use verbose make file
SET ( CMAKE_VERBOSE_MAKEFILE TRUE )

set (PLUGIN_NAME "partio4Maya")
set (PLUGIN_SUFFIX ".so")

# update the base dir to where your copy of maya resides
SET(MAYA_BASE_DIR "/usr/autodesk/maya2012.17-x64/")
SET(MAYA_INC_DIR ${MAYA_BASE_DIR}/include)
SET(MAYA_LIB_DIR ${MAYA_BASE_DIR}/lib)

## if building from the default partio contrib dir you can leave this alone
SET (PARTIO_INCLUDE "../../include" )
SET (PARTIO_LIB   "../../lib")

set (STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

include_directories (${STD_INCLUDES} ${partio4MayaINCLUDES} ${MAYA_INC_DIR} ${PARTIO_INCLUDE})
link_directories  ( ${STD_LIB} ${MAYA_LIB_DIR} ${PARTIO_LIB})

add_definitions(-D_BOOL)
add_definitions(-DREQUIRE_IOSTREAM)
add_definitions(-fPIC)
add_definitions(-O3)
add_definitions(-pthread)


FILE(GLOB partio4MayaCPP "*.cpp")
FILE(GLOB partio4MayaINCLUDES "*.h")

SET(TARGET_H ${partio4MayaINCLUDES})
SET(TARGET_SRC ${partio4MayaCPP})

SET(OPENGL_gl_LIBRARY "GL")
SET(OPENGL_glu_LIBRARY "GLU")



IF (WIN32)
	   SET(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)
	   SET(PARTIO_LIB  libpartio.a)
	ELSE()
		SET(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)
		SET(PARTIO_LIB  libpartio.a)
	ENDIF()


ADD_LIBRARY(${PLUGIN_NAME} SHARED ${TARGET_H} ${TARGET_SRC})
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ./build)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(${PLUGIN_NAME} ${PARTIO_LIB} ${MAYA_LIBS} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})

