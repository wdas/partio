# PARTIO SOFTWARE
# Copyright 2010 Disney Enterprises, Inc. All rights reserved
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
#
# * The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
# Studios" or the names of its contributors may NOT be used to
# endorse or promote products derived from this software without
# specific prior written permission from Walt Disney Pictures.
#
# Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
# IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

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
SET( PARTIO_HOME "L:/partio" )

# output executable and library path
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
SET(INCLUDE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/include)

## Setup platform specific helper defines build variants
IF(WIN32)
	ADD_DEFINITIONS (-DPARTIO_WIN32)

	# these are the defaults included with partio
	SET( ZLIB_INCLUDE_DIR "${PARTIO_HOME}/winReqLibs/zlib-win_x64/include")
	SET( ZLIB_LIBRARY_DIR "${PARTIO_HOME}/winReqLibs/zlib-win_x64/lib")
	SET (ZLIB_LIBRARY "${ZLIB_LIBRARY_DIR}/zlib.lib")
	SET( GLUT_INCLUDE_DIR "${PARTIO_HOME}/winReqLibs/freeglut-2.8.0-1/include")
	SET( GLUT_LIBRARY_DIR "${PARTIO_HOME}/winReqLibs/freeglut-2.8.0-1/lib/x64")
	SET( SWIG_EXECUTABLE "${PARTIO_HOME}/winReqLibs/swigwin-2.0.4/swig.exe")

ELSE(WIN32)
	ADD_DEFINITIONS (-Wextra -Wno-unused-parameter)
	SET( CMAKE_CXX_FLAGS "-fPIC")
ENDIF(WIN32)

# we want to push the partio lib up to the main dir
	SET( LIBRARY_OUTPUT_PATH "${PARTIO_HOME}/lib")

## Set install location
EXECUTE_PROCESS(COMMAND sh -c "echo `uname`-`uname -r | cut -d'-' -f1`-`uname -m`" OUTPUT_VARIABLE VARIANT_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
#EXECUTE_PROCESS(COMMAND uname OUTPUT_VARIABLE VARIANT_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/${VARIANT_DIRECTORY}")

## Search for needed  libraries
find_package(SWIG REQUIRED)

find_package(GLUT REQUIRED)
IF(GLUT_FOUND)
ELSE(GLUT_FOUND)
	INCLUDE_DIRECTORIES ( ${GLUT_INCLUDE_DIR} )
	LINK_DIRECTORIES ( ${GLUT_LIBRARY_DIR} )	
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
	SET( PARTIO_LIBRARIES ${ZLIB_LIBRARY} partio ) 
ELSEIF (UNIX)
	SET( PARTIO_LIBRARIES glut z partio )
ENDIF()
# make it so partio can be found
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/lib )

# install partio headers for use by other programs
install(
	FILES
	${CMAKE_SOURCE_DIR}/src/lib/PartioAttribute.h
	${CMAKE_SOURCE_DIR}/src/lib/Partio.h
	${CMAKE_SOURCE_DIR}/src/lib/PartioIterator.h
	DESTINATION ${INCLUDE_OUTPUT_PATH}
	)


## Traverse subdirectories
ADD_SUBDIRECTORY (src/lib)
ADD_SUBDIRECTORY (src/tools)
ADD_SUBDIRECTORY (src/py)
ADD_SUBDIRECTORY (src/tests)
ADD_SUBDIRECTORY (src/doc)
ADD_SUBDIRECTORY (contrib)


