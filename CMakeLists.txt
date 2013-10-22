# Copyright Disney Enterprises, Inc. All rights reserved.
#
# This license governs use of the accompanying software. If you use the software, you
# accept this license. If you do not accept the license, do not use the software.

# 1. Definitions
# The terms "reproduce," "reproduction," "derivative works," and "distribution" have
# the same meaning here as under U.S. copyright law. A "contribution" is the original
# software, or any additions or changes to the software. A "contributor" is any person
# that distributes its contribution under this license. "Licensed patents" are a
# contributor's patent claims that read directly on its contribution.

# 2. Grant of Rights
# (A) Copyright Grant- Subject to the terms of this license, including the license
# conditions and limitations in section 3, each contributor grants you a non-exclusive,
# worldwide, royalty-free copyright license to reproduce its contribution, prepare
# derivative works of its contribution, and distribute its contribution or any derivative
# works that you create.
# (B) Patent Grant- Subject to the terms of this license, including the license
# conditions and limitations in section 3, each contributor grants you a non-exclusive,
# worldwide, royalty-free license under its licensed patents to make, have made,
# use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the
# software or derivative works of the contribution in the software.

# 3. Conditions and Limitations
# (A) No Trademark License- This license does not grant you rights to use any
# contributors' name, logo, or trademarks.
# (B) If you bring a patent claim against any contributor over patents that you claim
# are infringed by the software, your patent license from such contributor to the
# software ends automatically.
# (C) If you distribute any portion of the software, you must retain all copyright,
# patent, trademark, and attribution notices that are present in the software.
# (D) If you distribute any portion of the software in source code form, you may do
# so only under this license by including a complete copy of this license with your
# distribution. If you distribute any portion of the software in compiled or object code
# form, you may only do so under a license that complies with this license.
# (E) The software is licensed "as-is." You bear the risk of using it. The contributors
# give no express warranties, guarantees or conditions. You may have additional
# consumer rights under your local laws which this license cannot change.
# To the extent permitted under your local laws, the contributors exclude the
# implied warranties of merchantability, fitness for a particular purpose and non-
# infringement.

## CMake compatibility issues: don't modify this, please!
CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)

if(COMMAND cmake_policy)
     cmake_policy(SET CMP0003 NEW)
	 cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
endif(COMMAND cmake_policy)

## allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )
## Use verbose make file
SET ( CMAKE_VERBOSE_MAKEFILE TRUE )


## Choose build options
# Disney specific method of choosing variant
IF("$ENV{FLAVOR}" STREQUAL "optimize")
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "type of build" FORCE)
ENDIF("$ENV{FLAVOR}" STREQUAL "optimize")
IF("$ENV{FLAVOR}" STREQUAL "debug")
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "type of build" FORCE)
ENDIF("$ENV{FLAVOR}" STREQUAL "debug")
# Set to release if nothing else defined
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

## project name & version
PROJECT( partio )
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 0 )

## CHANGE THIS TO WHERE YOU PUT THE PARTIO MAIN DIRECTORY
SET( PARTIO_HOME ${CMAKE_SOURCE_DIR} )

## Setup platform specific helper defines build variants
IF(WIN32)
	ADD_DEFINITIONS (-DPARTIO_WIN32)

	# these are the defaults included with partio
	SET( ZLIB_INCLUDE_DIR "${PARTIO_HOME}/winReqLibs/zlib-win_x64/include")
	SET( ZLIB_LIBRARY_DIR "${PARTIO_HOME}/winReqLibs/zlib-win_x64/lib")
	SET( ZLIB_LIBRARY "${ZLIB_LIBRARY_DIR}/zlib.lib")
	SET( GLUT_ROOT_PATH "${PARTIO_HOME}/winReqLibs/freeglut-2.8.0-1")
	SET( GLUT_INCLUDE_PATH "${PARTIO_HOME}/winReqLibs/freeglut-2.8.0-1/include")
	SET( GLUT_LIBRARY_PATH "${PARTIO_HOME}/winReqLibs/freeglut-2.8.0-1/lib/x64")
	SET( SWIG_EXECUTABLE "${PARTIO_HOME}/winReqLibs/swigwin-2.0.4/swig.exe")
	SET( PARTIO_BUILD_PLATFORM "windows_x86_64")
	set(CMAKE_MODULE_PATH ${PARTIO_HOME}/cmake)

ELSE(WIN32)
	ADD_DEFINITIONS (-Wextra -Wno-unused-parameter)
	SET( CMAKE_CXX_FLAGS "-fPIC")
ENDIF(WIN32)

## Set install location
IF (UNIX)
	#EXECUTE_PROCESS(COMMAND sh -c "echo `uname`-`uname -r | cut -d'-' -f1`-`uname -m`" OUTPUT_VARIABLE VARIANT_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
    EXECUTE_PROCESS(COMMAND sh -c "echo `uname`-`uname -m`" OUTPUT_VARIABLE VARIANT_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
	SET(PARTIO_BUILD_PLATFORM "${VARIANT_DIRECTORY}")
	MESSAGE(STATUS "Build Platform: ${PARTIO_BUILD_PLATFORM}")

ENDIF()

# output executable and library path
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/outputs/lib/${PARTIO_BUILD_PLATFORM})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/outputs/bin/${PARTIO_BUILD_PLATFORM})

# output executable and library path
SET(EXECUTABLE_INSTALL_PATH ${CMAKE_SOURCE_DIR}/build-${PARTIO_BUILD_PLATFORM}/bin/${PARTIO_BUILD_PLATFORM})
SET(LIBRARY_INSTALL_PATH ${CMAKE_SOURCE_DIR}/build-${PARTIO_BUILD_PLATFORM}/lib/${PARTIO_BUILD_PLATFORM})
SET(INCLUDE_INSTALL_PATH ${CMAKE_SOURCE_DIR}/build-${PARTIO_BUILD_PLATFORM}/include)

## Search for needed  libraries
find_package(SWIG REQUIRED)

## Search for useful libraries
find_package(GLUT REQUIRED)
IF(GLUT_FOUND)
	INCLUDE_DIRECTORIES( ${GLUT_INCLUDE_PATH} )
	LINK_DIRECTORIES (${GLUT_ROOT_PATH}/lib/x64)
ELSE(GLUT_FOUND)
	INCLUDE_DIRECTORIES( ${GLUT_INCLUDE_PATH} )
	LINK_DIRECTORIES ( ${GLUT_LIBRARY_PATH} )
ENDIF(GLUT_FOUND)

find_package(OpenGL REQUIRED)

find_package(ZLIB REQUIRED)
IF(ZLIB_FOUND)
    ADD_DEFINITIONS (-DPARTIO_USE_ZLIB)
    INCLUDE_DIRECTORIES ( ${ZLIB_INCLUDE_DIR} )
    LINK_DIRECTORIES ( ${ZLIB_LIBRARY_DIR} )
ELSE(ZLIB_FOUND)
    ADD_DEFINITIONS (-DPARTIO_USE_ZLIB)
    INCLUDE_DIRECTORIES ( ${ZLIB_INCLUDE_DIR} )
    LINK_DIRECTORIES ( ${ZLIB_LIBRARY_DIR} )
    SET (ZLIB_LIBRARY "${ZLIB_LIBRARY_DIR}/zlib.lib")
ENDIF(ZLIB_FOUND)

## Make modules able to see partio library
# Setup environment variable to link partio
IF (WIN32)
	SET( PARTIO_LIBRARIES freeglut zlib partio )
ELSEIF(APPLE)
	SET( PARTIO_LIBRARIES ${GLUT_glut_LIBRARY} ${GLUT_cocoa_LIBRARY} ${ZLIB_LIBRARY} partio )
ELSEIF (UNIX)
	SET( PARTIO_LIBRARIES glut z partio )
ENDIF()
# make it so partio can be found
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/lib )

## Traverse subdirectories
ADD_SUBDIRECTORY (src/lib)
ADD_SUBDIRECTORY (src/tools)
ADD_SUBDIRECTORY (src/py)
ADD_SUBDIRECTORY (src/tests)
#ADD_SUBDIRECTORY (src/houdini)
#ADD_SUBDIRECTORY (src/doc)
ADD_SUBDIRECTORY (contrib)


