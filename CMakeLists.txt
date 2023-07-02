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

cmake_minimum_required(VERSION 3.15.0)
project(partio LANGUAGES CXX)
set(PARTIO_VERSION_MAJOR "1")

option(PARTIO_GTEST_ENABLED "Enable GTest for tests" OFF)
option(PARTIO_ORIGIN_RPATH "Enable ORIGIN rpath in the installed libraries" OFF)

if(WIN32 OR APPLE)
    option(PARTIO_USE_GLVND "Use GLVND for OpenGL" OFF)
    option(PARTIO_BUILD_SHARED_LIBS "Enabled shared libraries" OFF)
else()
    option(PARTIO_USE_GLVND "Use GLVND for OpenGL" ON)
    option(PARTIO_BUILD_SHARED_LIBS "Enabled shared libraries" ON)
endif()

# Enable C++11
if (DEFINED ENV{CXXFLAGS_STD})
    set(CXXFLAGS_STD $ENV{CXXFLAGS_STD})
else()
    set(CXXFLAGS_STD "c++14")
endif()
# Transform "c++17" into "17".  "-1" means "rest of the string".
if (NOT DEFINED WDAS_CXX_STANDARD)
    string(SUBSTRING "${CXXFLAGS_STD}" 3 -1 WDAS_CXX_STANDARD)
endif()

set(CMAKE_CXX_STANDARD "${WDAS_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_FIND_PACKAGE_RESOLVE_SYMLINKS ON)
set(CMAKE_INSTALL_MESSAGE LAZY)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
if (PARTIO_ORIGIN_RPATH)
    # this is use for building the python wheel
    set(CMAKE_SKIP_RPATH OFF)
    set(CMAKE_BUILD_RPATH "$ORIGIN")
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
else ()
    set(CMAKE_SKIP_RPATH ON)
endif()

## Setup platform specific helper defines build variants
if (WIN32)
    if (MSVC)
        add_definitions(-DPARTIO_WIN32)
    endif()
    add_definitions(-D_USE_MATH_DEFINES)
else()
    add_compile_options(-Wextra -Wno-unused-parameter)
endif()

if (APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    # Detect the build type from the $FLAVOR environment variable
    # Default to optimized Release builds when unspecified.
    if ("$ENV{FLAVOR}" MATCHES "debug")
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "type of build" FORCE)
    elseif ("$ENV{FLAVOR}" MATCHES "profile")
        set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "type of build" FORCE)
    else ()
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "type of build" FORCE)
    endif()
endif()

include(CTest)
enable_testing()

## Set install location
include(GNUInstallDirs)
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if (WIN32)
        set(VARIANT_DIRECTORY "Windows-x86_64")
    else()
        execute_process(COMMAND sh -c "echo `uname`-`uname -r | cut -d'-' -f1`-`uname -m`" OUTPUT_VARIABLE VARIANT_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/${VARIANT_DIRECTORY}")
endif()

# Prefer libglvnd for OpenGL
set(OpenGL_GL_PREFERENCE GLVND)

## Search for useful libraries
find_package(Threads REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(ZLIB)
if (ZLIB_FOUND)
    add_definitions(-DPARTIO_USE_ZLIB)
else()
    set(ZLIB_LIBRARY "")
endif()

# Make modules able to see partio library
set(PARTIO_LIBRARIES partio ${ZLIB_LIBRARY})

## Traverse subdirectories
add_subdirectory(src/lib)
add_subdirectory(src/tools)
add_subdirectory(src/py)
add_subdirectory(src/doc)

if (${PARTIO_GTEST_ENABLED})
    set(GTEST_LOCATION "/usr" CACHE STRING "gtest installation prefix")
    set(GTEST_INCLUDE_PATH ${GTEST_LOCATION}/include)
    set(GTEST_LIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(GTEST_LINK_PATH ${GTEST_LOCATION}/${GTEST_LIBDIR} CACHE STRING "gtest library directory")

    add_subdirectory(src/tests)
endif()
