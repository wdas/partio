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

CMAKE_MINIMUM_REQUIRED( VERSION 3.8.0 )
SET( CMAKE_SKIP_RPATH TRUE )

## project name & version
PROJECT( partio )
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 0 )
include(GNUInstallDirs)

# PARTIO_SE_ENABLED enables SeExpr support
option(PARTIO_SE_ENABLED "PARTIO_SE_ENABLED" false)

# Enable C++11
if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif ()
else ()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED YES)
    set(CMAKE_CXX_EXTENSIONS NO)
endif ()

## Setup platform specific helper defines build variants
IF(WIN32)
  ADD_DEFINITIONS (-DPARTIO_WIN32 -D_USE_MATH_DEFINES)
ELSE(WIN32)
  ADD_DEFINITIONS (-Wextra -Wno-unused-parameter)
ENDIF(WIN32)

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

## Set install location
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    EXECUTE_PROCESS(COMMAND sh -c "echo `uname`-`uname -r | cut -d'-' -f1`-`uname -m`" OUTPUT_VARIABLE VARIANT_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/${VARIANT_DIRECTORY}")
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Prefer libglvnd for OpenGL
set(OpenGL_GL_PREFERENCE GLVND)

## Search for useful libraries
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB)
IF(ZLIB_FOUND)
    ADD_DEFINITIONS (-DPARTIO_USE_ZLIB)
ELSE(ZLIB_FOUND)
    SET (ZLIB_LIBRARY "")
ENDIF(ZLIB_FOUND)

option(BUILD_SHARED_LIBS "Enabled shared libraries" ON)

## Make modules able to see partio library
# Setup environment variable to link partio
SET( PARTIO_LIBRARIES partio ${ZLIB_LIBRARY} )

if (${PARTIO_SE_ENABLED})
    add_definitions(-DPARTIO_SE_ENABLED=1)
    SET(SEEXPR_BASE "c:/aselle/seexpr-inst" CACHE STRING "path to SeExpr")
    SET(SEEXPR_INCLUDE_PATH ${SEEXPR_BASE}/include)
    SET(SEEXPR_LIBDIR ${CMAKE_INSTALL_LIBDIR})
    SET(SEEXPR_LINK_PATH ${SEEXPR_BASE}/${SEEXPR_LIBDIR})
    SET(SEEXPR_LIBS "SeExpr")
endif ()


## Traverse subdirectories
ADD_SUBDIRECTORY (src/lib)
ADD_SUBDIRECTORY (src/tools)
ADD_SUBDIRECTORY (src/py)
ADD_SUBDIRECTORY (src/tests)
ADD_SUBDIRECTORY (src/doc)
