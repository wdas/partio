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

FILE(GLOB io_cpp "io/*.cpp" )
FILE(GLOB 3rdParty_io_cpp "io/3rdParty/*/*.cpp" )
FILE(GLOB core_cpp "core/*.cpp")
ADD_LIBRARY (partio ${3rdParty_io_cpp} ${io_cpp} ${core_cpp})

FILE(GLOB public_includes "*.h")

INSTALL (TARGETS partio DESTINATION ${LIBRARY_INSTALL_PATH})
INSTALL( FILES  ${public_includes} 	DESTINATION ${INCLUDE_INSTALL_PATH})
