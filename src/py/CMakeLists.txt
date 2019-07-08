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



FIND_PACKAGE(SWIG)
IF(SWIG_FOUND) 

FIND_PACKAGE(PythonInterp)
IF(PythonInterp_FOUND)

INCLUDE(${SWIG_USE_FILE})

# Workaround to find python2/3 until we can use find_package(Python2) / find_package(Python3)
# with newer versions of cmake.
EXECUTE_PROCESS(COMMAND
    ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig as sc; print(sc.get_config_var('INCLUDEPY'))"
    OUTPUT_VARIABLE PYTHON_INCLUDE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)

EXECUTE_PROCESS(COMMAND
    ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig as sc; print(sc.get_config_var('LIBDIR'))"
    OUTPUT_VARIABLE PYTHON_LIBRARY_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

EXECUTE_PROCESS(COMMAND
    ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig as sc; print(sc.get_config_var('LDLIBRARY'))"
    OUTPUT_VARIABLE PYTHON_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE)

SET(PYTHON_LIBRARIES ${PYTHON_LIBRARY_DIR}/${PYTHON_LIBRARY})

INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src/lib)

SET(CMAKE_SWIG_FLAGS "")

# setup output path
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/py)
SET(CMAKE_SWIG_OUTDIR ${PROJECT_BINARY_DIR}/py)

SET_SOURCE_FILES_PROPERTIES(partio.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(partio.i PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_LIBRARY(partio LANGUAGE python SOURCES partio.i)
SWIG_LINK_LIBRARIES(partio ${PYTHON_LIBRARIES} partio)

SET(PYTHON_DEST "${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages" )
INSTALL(TARGETS _partio DESTINATION ${PYTHON_DEST})
INSTALL(FILES ${CMAKE_BINARY_DIR}/${outdir}/py/partio.py DESTINATION ${PYTHON_DEST})
INSTALL(FILES partio.i DESTINATION ${CMAKE_INSTALL_DATADIR}/swig)

ENDIF(PythonInterp_FOUND)
ENDIF(SWIG_FOUND)
