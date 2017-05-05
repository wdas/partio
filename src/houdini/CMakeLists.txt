# PARTIO SOFTWARE
# Copyright 2013 Disney Enterprises, Inc. All rights reserved
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

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(Houdini)

if(HOUDINI_FOUND)

	SET (HOUDINI_LIBS
	     HoudiniUI
	     HoudiniOPZ
	     HoudiniOP3
	     HoudiniOP2
	     HoudiniOP1
	     HoudiniSIM
	     HoudiniGEO
	     HoudiniPRM
	     HoudiniUT)

	ADD_DEFINITIONS (${HOUDINI_DEFINITIONS} -DHOUDINI_DEFINES -DHOUDINI_API_VERSION=${HOUDINI_VERSION}  -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DMAKING_DSO)

	LINK_DIRECTORIES( ${HOUDINI_LIBRARY_DIRS} )
	INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/lib ${HOUDINI_INCLUDE_DIRS} )

	ADD_LIBRARY(partio4Houdini SHARED SOP_FromPartio.cpp ROP_ToPartio.cpp initialize.cpp)
	TARGET_LINK_LIBRARIES(partio4Houdini ${PARTIO_LIBRARIES} ${HOUDINI_LIBS})

	INSTALL(TARGETS partio4Houdini
	        DESTINATION houdini )

endif (HOUDINI_FOUND)


