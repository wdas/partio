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

include_directories(${CMAKE_SOURCE_DIR}/src/lib)
link_directories(${CMAKE_BINARY_DIR}/src/lib)

if (GLUT_FOUND AND OPENGL_FOUND)
    add_executable(partview partview.cpp)
    target_link_libraries(
        partview
        PRIVATE
        ${PARTIO_LIBRARIES}
        GLUT::GLUT
        OpenGL::OpenGL
        OpenGL::GLU
    )
    install(TARGETS partview DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

add_executable(partinfo partinfo.cpp)
target_link_libraries(partinfo ${PARTIO_LIBRARIES})

add_executable(partconvert partconvert.cpp)
target_link_libraries(partconvert ${PARTIO_LIBRARIES})

add_executable(partattr partattr.cpp)
target_link_libraries(partattr ${PARTIO_LIBRARIES})

install(TARGETS partattr partconvert partinfo DESTINATION ${CMAKE_INSTALL_BINDIR})

install(PROGRAMS partedit.py DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME partedit)
install(PROGRAMS partjson.py DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME partjson)
install(PROGRAMS partinspect.py DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME partinspect)

set(PYTHON_DEST "${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages" )
install(FILES partedit.py partjson.py partinspect.py DESTINATION ${PYTHON_DEST})
