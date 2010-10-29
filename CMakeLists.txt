# CMake compatibility issues: don't modify this, please!
CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)
# allow more human readable "if then else" constructs
SET( CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE )

#SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/Linux") 
#SET(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build") 

PROJECT( partio )

# Default to release

#IF(NOT CMAKE_BUILD_TYPE)
#  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING
#      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
#      FORCE)
#ENDIF(NOT CMAKE_BUILD_TYPE)

IF($ENV{FLAVOR} EQUAL "optimize")
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING FORCE)
ENDIF($ENV{FLAVOR} EQUAL "optimize")
IF($ENV{FLAVOR} EQUAL "debug")
    SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING FORCE)
ENDIF($ENV{FLAVOR} EQUAL "debug")

SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/${CMAKE_BUILD_TYPE}")

# Search for useful libraries
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)

# Setup build variants
ADD_DEFINITIONS (-Wall)
#SET(CMAKE_BUILD_TYPE release)


SET( PARTIO_LIBRARIES ${ZLIB_LIBRARY} partio )

# project version
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 0 )

SET ( CMAKE_VERBOSE_MAKEFILE TRUE )
SET( CMAKE_CXX_FLAGS "-fPIC")

# make it so partio can be found
include_directories(
   ${CMAKE_SOURCE_DIR}/src/lib
)

ADD_SUBDIRECTORY (src/lib)
ADD_SUBDIRECTORY (src/tools)
ADD_SUBDIRECTORY (src/py)
ADD_SUBDIRECTORY (src/tests)


