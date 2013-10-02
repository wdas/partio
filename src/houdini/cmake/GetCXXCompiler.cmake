#
# Get information about the C++ compiler.
#
# Out:
#  ${_PREFIX}_COMPILER_BINARY
#  ${_PREFIX}_COMPILER_VERSION
#
# Any variable that could not be determined will be set to 'FALSE'
#
function(GetCXXCompiler _PREFIX)
    set(${_PREFIX}_COMPILER_BINARY ${CMAKE_CXX_COMPILER} PARENT_SCOPE)
    set(${_PREFIX}_COMPILER_VERSION FALSE PARENT_SCOPE)

    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
        OUTPUT_VARIABLE _VER_PROC_OUT RESULT_VARIABLE _PROC_RES ERROR_QUIET
    )
    
    if(_PROC_RES)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} --version
            OUTPUT_VARIABLE _VER_PROC_OUT RESULT_VARIABLE _PROC_RES ERROR_QUIET
        )
    endif(_PROC_RES)
    
    if(NOT _PROC_RES)
        string(REGEX MATCH "[0-9]+(\\.[0-9]+)*" _COMPILER_VERSION ${_VER_PROC_OUT})
        if(_COMPILER_VERSION)
            set(${_PREFIX}_COMPILER_VERSION ${_COMPILER_VERSION} PARENT_SCOPE)
        endif(_COMPILER_VERSION)
    endif(NOT _PROC_RES)    
endfunction()



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



