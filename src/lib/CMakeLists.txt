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

file(GLOB io_cpp "io/*.cpp")
file(GLOB core_cpp "core/*.cpp")

add_library(partio ${io_cpp} ${core_cpp})
set_target_properties(partio PROPERTIES OUTPUT_NAME partio POSITION_INDEPENDENT_CODE ON)

target_include_directories(partio
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if (ZLIB_FOUND)
    target_include_directories(partio
        PUBLIC
            $<INSTALL_INTERFACE:${ZLIB_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${ZLIB_INCLUDE_DIR}>)
    target_link_libraries(partio PUBLIC ${ZLIB_LIBRARY})
endif (ZLIB_FOUND)

install(TARGETS partio DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (SEEXPR_BASE)
    if (${PARTIO_SE_ENABLED})
        add_library(partioSe PartioSe.cpp)
        set_target_properties(partioSe PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_include_directories(partioSe
            PUBLIC
                $<INSTALL_INTERFACE:${SEEXPR_INCLUDE_PATH}>
                $<BUILD_INTERFACE:${SEEXPR_INCLUDE_DIR}>
            PRIVATE
                ${SEEXPR_INCLUDE_PATH})
        target_link_libraries(partioSe PUBLIC partio ${SEEXPR_LIB})
        install(TARGETS partioSe DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif ()
endif (SEEXPR_BASE)

file(GLOB public_includes "*.h")
install(FILES ${public_includes} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
