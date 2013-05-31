#
# In:
#  HOUDINI_ROOT (alternatively, make sure 'houdini' is on $PATH)
#  HOUDINI_DSO_TAG
#
# Out:
#  HOUDINI_FOUND
#  HOUDINI_VERSION
#  HOUDINI_MAJOR_VERSION
#  HOUDINI_MINOR_VERSION
#  HOUDINI_BUILD_VERSION
#  HOUDINI_INCLUDE_DIRS
#  HOUDINI_LIBRARY_DIRS
#  HOUDINI_DEFINITIONS
#  HOUDINI_TOOLKIT_DIR
#

include(GetCXXCompiler)


find_program(HOUDINI_BINARY houdini PATHS ${HOUDINI_ROOT}/bin)
if(HOUDINI_BINARY STREQUAL "HOUDINI_BINARY-NOTFOUND" )
    find_program(HOUDINI_BINARY houdini PATHS $ENV{HFS}/bin)
endif(HOUDINI_BINARY STREQUAL "HOUDINI_BINARY-NOTFOUND" )


if(HOUDINI_BINARY STREQUAL "HOUDINI_BINARY-NOTFOUND" )
    set(HOUDINI_FOUND FALSE)
else(HOUDINI_BINARY STREQUAL "HOUDINI_BINARY-NOTFOUND" )
    set(HOUDINI_FOUND TRUE)
    get_filename_component(HOUDINI_BASE ${HOUDINI_BINARY} PATH)
    get_filename_component(HOUDINI_BASE ${HOUDINI_BASE} PATH)
    set(HOUDINI_LIBRARY_DIRS ${HOUDINI_BASE}/dsolib)  
    set(HOUDINI_INCLUDE_DIRS ${HOUDINI_BASE}/toolkit/include)
    set(HOUDINI_TOOLKIT_DIR ${HOUDINI_BASE}/toolkit)
    
    file(READ "${HOUDINI_INCLUDE_DIRS}/UT/UT_Version.h" _HOU_UTVER_CONTENTS)
    string(REGEX REPLACE ".*#define UT_MAJOR_VERSION_INT ([0-9]+).*" "\\1" HOUDINI_MAJOR_VERSION "${_HOU_UTVER_CONTENTS}")
    string(REGEX REPLACE ".*#define UT_MINOR_VERSION_INT ([0-9]+).*" "\\1" HOUDINI_MINOR_VERSION "${_HOU_UTVER_CONTENTS}")
    string(REGEX REPLACE ".*#define UT_BUILD_VERSION_INT ([0-9]+).*" "\\1" HOUDINI_BUILD_VERSION "${_HOU_UTVER_CONTENTS}")
    set(HOUDINI_VERSION "${HOUDINI_MAJOR_VERSION}.${HOUDINI_MINOR_VERSION}.${HOUDINI_BUILD_VERSION}")
    
    set(HOUDINI_DEFINITIONS "")

    # general definitions
    list(APPEND HOUDINI_DEFINITIONS -DVERSION="$ENV{HOUDINI_VERSION}")
    list(APPEND HOUDINI_DEFINITIONS -DMAKING_DSO )
    if(NOT HOUDINI_DSO_TAG)
        set(HOUDINI_DSO_TAG "unknown")
    endif(NOT HOUDINI_DSO_TAG)    
    list(APPEND HOUDINI_DEFINITIONS -DUT_DSO_TAGINFO="${HOUDINI_DSO_TAG}" )
    
    GetCXXCompiler(_HOU)
    set(_HOU_NEED_SPEC_STORAGE TRUE)
    if(_HOU_COMPILER_VERSION)
        if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
            if(_HOU_COMPILER_VERSION VERSION_GREATER 4.2)
                set(_HOU_NEED_SPEC_STORAGE FALSE)
            endif(_HOU_COMPILER_VERSION VERSION_GREATER 4.2)
        endif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
    endif(_HOU_COMPILER_VERSION)
    
    if(_HOU_NEED_SPEC_STORAGE)
        list(APPEND HOUDINI_DEFINITIONS -DNEED_SPECIALIZATION_STORAGE )
    endif(_HOU_NEED_SPEC_STORAGE)
    
    # platform-specific definitions
    if( APPLE )
        # OSX
    elseif( WIN32 )
        # Win32
        list(APPEND HOUDINI_DEFINITIONS -DI386)
        list(APPEND HOUDINI_DEFINITIONS -DWIN32)
        list(APPEND HOUDINI_DEFINITIONS -DSWAP_BITFIELDS)
        list(APPEND HOUDINI_DEFINITIONS -DDLLEXPORT=__declspec\(dllexport\) )
        list(APPEND HOUDINI_DEFINITIONS -DSESI_LITTLE_ENDIAN)
    else( APPLE )
        # Linux common
        list(APPEND HOUDINI_DEFINITIONS -DLINUX)
        list(APPEND HOUDINI_DEFINITIONS -D_GNU_SOURCE)
        list(APPEND HOUDINI_DEFINITIONS -DDLLEXPORT= )
        list(APPEND HOUDINI_DEFINITIONS -DSESI_LITTLE_ENDIAN)
        list(APPEND HOUDINI_DEFINITIONS -DENABLE_THREADS)
        list(APPEND HOUDINI_DEFINITIONS -DUSE_PTHREADS)
        list(APPEND HOUDINI_DEFINITIONS -DENABLE_UI_THREADS)
        list(APPEND HOUDINI_DEFINITIONS -DGCC3)
        list(APPEND HOUDINI_DEFINITIONS -DGCC4)
        list(APPEND HOUDINI_DEFINITIONS -Wno-deprecated)
        
        # Linux 64 bit
        list(APPEND HOUDINI_DEFINITIONS -DAMD64)
        list(APPEND HOUDINI_DEFINITIONS -DSIZEOF_VOID_P=8)
    endif( APPLE )

endif(HOUDINI_BINARY STREQUAL "HOUDINI_BINARY-NOTFOUND" )


if( Houdini_FIND_REQUIRED AND NOT HOUDINI_FOUND )
    message(FATAL_ERROR "Could not find houdini")
endif( Houdini_FIND_REQUIRED AND NOT HOUDINI_FOUND )


#
# Copyright 2012, Allan Johns
#
# vfxcmake is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# vfxcmake is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with vfxcmake.  If not, see <http://www.gnu.org/licenses/>.
#
