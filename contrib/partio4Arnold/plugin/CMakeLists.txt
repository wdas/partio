##### THIS WILL NEED SOME HELP WITH  NON-Linux compiles

## CMake compatibility issues: don't modify this, please!
CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )

# Check if the GCC version matches the required version on Linux
IF ( DEFINED USE_MAYA_CERTIFIED_COMPILER AND USE_MAYA_CERTIFIED_COMPILER )
  IF ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	IF ( CMAKE_COMPILER_IS_GNUCC )
      EXECUTE_PROCESS ( COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION )
      MESSAGE ( "GCC_VERSION is ${GCC_VERSION}" )
      STRING ( REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
      LIST ( GET GCC_VERSION_COMPONENTS 0 GCC_VERSION_MAJOR )
      LIST ( GET GCC_VERSION_COMPONENTS 1 GCC_VERSION_MINOR )
      LIST ( GET GCC_VERSION_COMPONENTS 2 GCC_VERSION_PATCH )
      IF (${GCC_VERSION_MAJOR} MATCHES "4"
          AND
          ${GCC_VERSION_MINOR} MATCHES "1"
          AND
          ${GCC_VERSION_PATCH} MATCHES "2"
          )
		MESSAGE ("CORRECT VERSION OF COMPILER FOR MAYA PLUGINS")
      ELSE ()
		MESSAGE (FATAL_ERROR "INCORRECT VERSION OF COMPILER FOR MAYA PLUGINS, EXPECTING GCC 4.1.2")
      ENDIF()
	ENDIF ()
  ENDIF ()
ENDIF ()


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

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)


MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
## Use verbose make file
SET ( CMAKE_VERBOSE_MAKEFILE TRUE )

set (PLUGIN_NAME "partio")
set (CMAKE_PROJECT_NAME "partioVizTranslator")

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
find_package(Maya)

## if building from the default partio contrib dir you can leave this alone
SET (PARTIO_INCLUDE "../../../include" )
SET (PARTIO_LIBDIR   "../../../lib")

set (STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

set (MTOA_DEVEL_INCLUDE "/Volumes/sv-dev01/devRepo/johnc/arnold/mtoa/plugins/mtoa")

include_directories (${STD_INCLUDES} ${partio4ArnoldINCLUDES} ${MAYA_INCLUDE_DIR} ${ARNOLD_INCLUDE_DIR} ${MTOA_DEVEL_INCLUDE} ${MTOA_INCLUDE_DIR} ${PARTIO_INCLUDE})
link_directories  ( ${STD_LIB} ${MAYA_LIBRARY_DIR} ${MtoA_mtoa_api_LIBRARY} ${ARNOLD_LIBRARY}  ${PARTIO_LIBDIR} ${ZLIB_LIBRARY_DIR})

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

TARGET_LINK_LIBRARIES(${PLUGIN_NAME} ${ZLIB}  ${MAYA_LIBS}  ${PARTIO_LIB} ${ARNOLD_LIBRARY} ${MtoA_mtoa_api_LIBRARY} )

#INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/ DESTINATION contrib/partio4 )

