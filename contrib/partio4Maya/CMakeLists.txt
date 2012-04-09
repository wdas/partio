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


MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
## Use verbose make file
SET ( CMAKE_VERBOSE_MAKEFILE TRUE )

set (PLUGIN_NAME "partio4Maya")
set (PLUGIN_SUFFIX ".so")
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

if (LINUX)
	set (PLUGIN_SUFFIX ".so")
	set (CMAKE_SHARED_LIBRARY_SUFFIX ".so")
	set (SUFFIX ".so")
ENDIF()
	

# update the base dir to where your copy of maya resides
SET(MAYA_VERSION "2012-x64")
if(WIN32)
	SET(MAYA_BASE_DIR "C:/Program Files/Autodesk/Maya${MAYA_VERSION}")
else(WIN32)
	SET(MAYA_BASE_DIR "/usr/autodesk/maya${MAYA_VERSION}")
ENDIF()
if(APPLE)
	SET(MAYA_BASE_DIR "/Applications/Autodesk/maya2012.17")
ENDIF()

if (APPLE) 
	SET(MAYA_INC_DIR ${MAYA_BASE_DIR}/devkit/include)
	SET(MAYA_LIB_DIR ${MAYA_BASE_DIR}/Maya.app/Contents/MacOS)
else(APPLE)
	SET(MAYA_INC_DIR ${MAYA_BASE_DIR}/include)
	SET(MAYA_LIB_DIR ${MAYA_BASE_DIR}/lib)
ENDIF()



## if building from the default partio contrib dir you can leave this alone
SET (PARTIO_INCLUDE "../../include" )
SET (PARTIO_LIB   "../../lib")

set (STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

include_directories (${STD_INCLUDES} ${partio4MayaINCLUDES} ${MAYA_INC_DIR} ${PARTIO_INCLUDE})
link_directories  ( ${STD_LIB} ${MAYA_LIB_DIR} ${PARTIO_LIB})

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
else(APPLE)

	SET(OPENGL_gl_LIBRARY "GL")
	SET(OPENGL_glu_LIBRARY "GLU")
endif()


IF (WIN32)
	   SET(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)
	   SET(PARTIO_LIB  partio)
	ELSE()
		SET(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)
		SET(PARTIO_LIB  libpartio.a)
	ENDIF()


ADD_LIBRARY(${PLUGIN_NAME} SHARED ${TARGET_H} ${TARGET_SRC})
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ./build)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(${PLUGIN_NAME} ${ZLIB} ${PARTIO_LIB} ${MAYA_LIBS} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})

