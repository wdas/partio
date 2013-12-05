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

SET(CMAKE_CXX_WARNING_LEVEL 4)

MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
## Use verbose make file
SET ( CMAKE_VERBOSE_MAKEFILE TRUE )

set (PLUGIN_NAME "partioTranslator")
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


set (STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

include_directories(${STD_INCLUDES}
                    ${partio4ArnoldINCLUDES}
                    ${MAYA_INCLUDE_DIR}
                    ${ARNOLD_INCLUDE_DIR}
                    ${MTOA_INCLUDE_DIR})
link_directories(${STD_LIB}
                 ${MAYA_LIBRARY_DIR}
                 ${MTOA_mtoa_api_LIBRARY}
                 ${ARNOLD_LIBRARY}
                 ${ZLIB_LIBRARY_DIR})

# Maya CFLAGS
IF (APPLE)
  SET ( DEVKIT_LOCATION ${MAYA_LOCATION}/../../devkit )
  ADD_DEFINITIONS (
   -DMAC_PLUGIN
   -D_BOOL
   -DCC_GNU_
   -DOSMac_
   -DOSMacOSX_
   -DREQUIRE_IOSTREAM
    -DOSMac_MachO_
   -D_LANGUAGE_C_PLUS_PLUS
    -include "${DEVKIT_LOCATION}/include/maya/OpenMayaMac.h"
   -fno-gnu-keywords
   -fpascal-strings
    -D_DARWIN # required by MtoA
  )
ELSE (APPLE)
  ADD_DEFINITIONS (
   -DBits64_
   -m64
   -DUNIX
   -D_BOOL
   -DLINUX
   -DFUNCPROTO
   -D_GNU_SOURCE
   -DLINUX_64
   -fPIC
   -fno-strict-aliasing
   -DREQUIRE_IOSTREAM
   -Wno-deprecated
   -Wall
   -Wno-multichar
   -Wno-comment
   -Wno-sign-compare
   -funsigned-char
   -Wno-reorder
   -fno-gnu-keywords
   -ftemplate-depth-25
   -pthread
   -DNDEBUG
   -O3
   -D_LINUX # required by MtoA
  )
ENDIF (APPLE)

FILE(GLOB partio4ArnoldCPP "*.cpp")
FILE(GLOB partio4ArnoldINCLUDES "*.h")


SET(MAYA_LIBS
  ${MAYA_OpenMaya_LIBRARY}
  ${MAYA_Foundation_LIBRARY}
  ${MAYA_OpenMayaAnim_LIBRARY}
  ${MAYA_OpenMayaFX_LIBRARY}
  ${MAYA_OpenMayaUI_LIBRARY}
  ${MAYA_OpenMayaRender_LIBRARY})


ADD_LIBRARY(${PLUGIN_NAME} SHARED ${partio4ArnoldINCLUDES} ${partio4ArnoldCPP})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES(${PLUGIN_NAME}
                      ${ZLIB}
                      ${MAYA_LIBS}
					  ${PARTIO_LIBRARIES}
                      ${ARNOLD_LIBRARY}
                      ${MTOA_mtoa_api_LIBRARY})

#SET (PARTIO4ARNOLD_OUTPUT_PATH  ${CMAKE_SOURCE_DIR}/build-${PARTIO_BUILD_PLATFORM}/arnold/mtoa-$ENV{MTOA_VERSION}/${PARTIO_BUILD_PLATFORM}/extensions)

INSTALL( TARGETS ${PLUGIN_NAME}
         DESTINATION arnold/mtoa/extensions )

FILE(GLOB PARTIO4ARNOLD_SCRIPTS "*.py")
INSTALL( FILES ${PARTIO4ARNOLD_SCRIPTS}
         DESTINATION arnold/mtoa/extensions )

