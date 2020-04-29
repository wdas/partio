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

set(CMAKE_INSTALL_PARTIO_TESTDIR ${CMAKE_INSTALL_DATAROOTDIR}/partio/test)
find_library(GTEST_LIB gtest HINTS ${GTEST_LINK_PATH})

if ("${GTEST_LIB}" STREQUAL "GTEST_LIB-NOTFOUND")
    message(STATUS "${GTEST_LIB} not found in ${GTEST_LINK_PATH}, tests disabled")
else()
    link_directories(${CMAKE_BINARY_DIR}/src/lib ${GTEST_LINK_PATH})

    foreach(item testiterator testio testcache testclonecopy testcluster teststr makecircle makeline testkdtree testmerge)
        add_executable(${item} "${item}.cpp")
        target_include_directories(${item} PRIVATE ${GTEST_INCLUDE_PATH})
        target_link_libraries(
            ${item} ${PARTIO_LIBRARIES} ${GTEST_LIB} Threads::Threads)
        target_compile_definitions(${item} PRIVATE -DPARTIO_DATA_DIR="${PROJECT_SOURCE_DIR}/src/data")
        install(TARGETS ${item} DESTINATION ${CMAKE_INSTALL_PARTIO_TESTDIR})
        add_test(NAME ${item} COMMAND ${item})
    endforeach(item)

    install(PROGRAMS testpartjson.py DESTINATION ${CMAKE_INSTALL_PARTIO_TESTDIR} RENAME testpartjson)
endif()
