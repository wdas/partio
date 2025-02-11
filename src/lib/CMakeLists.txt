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

set(core_cpp
    core/Particle.cpp
    core/ParticleCaching.cpp
    core/ParticleHeaders.cpp
    core/ParticleSimple.cpp
    core/ParticleSimpleInterleave.cpp)

set(io_cpp
    io/BGEO.cpp
    io/BHCLASSIC.cpp
    io/BIN.cpp
    io/GEO.cpp
    io/HCLASSIC.cpp
    io/MC.cpp
    io/ParticleIO.cpp
    io/PDA.cpp
    io/PDB.cpp
    io/PDC.cpp
    io/PRT.cpp
    io/PTC.cpp
    io/PTS.cpp
    io/RIB.cpp
    io/ZIP.cpp
)

if (bgeo_FOUND)
    list(APPEND io_cpp io/BJSON.cpp )
endif()

if(PARTIO_BUILD_SHARED_LIBS)
    set(PARTIO_LIBRARY_TYPE SHARED)
else()
    set(PARTIO_LIBRARY_TYPE STATIC)
endif()

add_library(partio ${PARTIO_LIBRARY_TYPE} ${io_cpp} ${core_cpp})
set_target_properties(partio PROPERTIES
    OUTPUT_NAME partio
    POSITION_INDEPENDENT_CODE ON
    SOVERSION ${PARTIO_VERSION_MAJOR})

target_include_directories(partio
    PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if (ZLIB_FOUND)
    target_link_libraries(partio PUBLIC ZLIB::ZLIB)
endif()

if (bgeo_FOUND)
    target_link_libraries(partio PUBLIC bgeo::bgeo)
    target_compile_definitions(partio PRIVATE MODERN_BGEO)
endif()

install(TARGETS partio DESTINATION ${CMAKE_INSTALL_LIBDIR})

file(GLOB public_includes "*.h")
install(FILES ${public_includes} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
