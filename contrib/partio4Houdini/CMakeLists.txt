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
cmake_minimum_required( VERSION 2.8.0 )

if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
    cmake_policy(SET CMP0005 NEW)
endif (COMMAND cmake_policy)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

set(PLUGIN_NAME "partio4Houdini")
set(CMAKE_PROJECT_NAME "partio4Houdini")

# no need to setup the compiler by hand, rez takes care of that
# no need to find houdini here, it's taken care at a higher level

message(STATUS "Found HOUDINI version ${HOUDINI_VERSION}")
message(STATUS "running command ${PROJECT_SOURCE_DIR}/contrib/cmake/hdk-config.py")

add_custom_command(OUTPUT sesitag.C
                   DEPENDS src/ROP_partio.C
                   COMMAND ${PROJECT_SOURCE_DIR}/contrib/cmake/hdk-config.py --tagfile=${CMAKE_BINARY_DIR}/contrib/partio4Houdini/sesitag.C)
set(HOUDINI_INCS ${HOUDINI_INCLUDE_DIRS} ${SYS_INCLUDE})
set(HOUDINI_LIBDIR ${HOUDINI_LIBRARY_DIRS})
set(HOUDINI_LIBS HoudiniUI HoudiniOPZ HoudiniOP3 HoudiniOP2 HoudiniOP1 HoudiniSIM HoudiniGEO HoudiniPRM HoudiniUT)
add_definitions(${HOUDINI_DEFINITIONS} -DHOUDINI_DEFINES -DHOUDINI_API_VERSION=$ENV{HOUDINI_VERSION}  -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DMAKING_DSO)

link_directories( ${HOUDINI_LIBDIR} )
include_directories(${CMAKE_SOURCE_DIR}/contrib/partio4Houdini/include ${HOUDINI_INCS})

add_library(${PLUGIN_NAME} SHARED src/partio_houdini.C src/SOP_partio.C src/ROP_partio.C sesitag.C)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN_NAME} ${PARTIO_LIBRARIES} ${HOUDINI_LIBS} ${HOUDINI_SYS_LIBS})

## install everything
set(HOUDINI_OUTPUT_PATH houdini/dso)
set(HOUDINI_ICONS_PATH  houdini/Icons)
if (BUILD_PARTIO_HOUDINI)
	set(HOUDINI_OUTPUT_PATH dso)
	set(HOUDINI_ICONS_PATH  Icons)
elseif (${LOCAL_INSTALL})
    set(HOUDINI_OUTPUT_PATH houdini/$ENV{HOUDINI_VERSION}/dso)
    set(HOUDINI_ICONS_PATH  houdini/$ENV{HOUDINI_VERSION}/Icons)
endif ()

install(TARGETS ${PLUGIN_NAME} DESTINATION ${HOUDINI_OUTPUT_PATH})

file(GLOB PARTIO4HOUDINI_ICONS "icons/*")
install(FILES ${PARTIO4HOUDINI_ICONS} DESTINATION ${HOUDINI_ICONS_PATH})
