# - Arnold finder module
# This module searches for a valid Arnold installation.
#
# Variables that will be defined:
# ARNOLD_FOUND              Defined if a Arnold installation has been detected
# ARNOLD_LIBRARY            Path to ai library (for backward compatibility)
# ARNOLD_LIBRARIES          Path to ai library
# ARNOLD_INCLUDE_DIR        Path to the include directory (for backward compatibility)
# ARNOLD_INCLUDE_DIRS       Path to the include directory
# ARNOLD_KICK               Path to kick
# ARNOLD_PYKICK             Path to pykick
# ARNOLD_MAKETX             Path to maketx
# ARNOLD_OSLC               Path to the osl compiler
# ARNOLD_OSL_HEADER_DIR     Path to the osl headers include directory (for backward compatibility)
# ARNOLD_OSL_HEADER_DIRS    Path to the osl headers include directory
# ARNOLD_VERSION_ARCH_NUM   Arch version of Arnold
# ARNOLD_VERSION_MAJOR_NUM  Major version of Arnold
# ARNOLD_VERSION_MINOR_NUM  Minor version of Arnold
# ARNOLD_VERSION_FIX        Fix version of Arnold
# ARNOLD_VERSION            Version of Arnold
# arnold_compile_osl        Function to compile / install .osl files
#   QUIET                   Quiet builds
#   VERBOSE                 Verbose builds
#   INSTALL                 Install compiled files into DESTINATION
#   INSTALL_SOURCES         Install sources into DESTINATION_SOURCES
#   OSLC_FLAGS              Extra flags for OSLC
#   DESTINATION             Destination for compiled files
#   DESTINATION_SOURCES     Destination for source files
#   INCLUDES                Include directories for oslc
#   SOURCES                 Source osl files
#
#
# Naming convention:
#  Local variables of the form _arnold_foo
#  Input variables from CMake of the form ARNOLD_FOO
#  Output variables of the form ARNOLD_FOO
#

if (EXISTS "$ENV{ARNOLD_HOME}")
    set(ARNOLD_HOME $ENV{ARNOLD_HOME})
endif ()

find_library(ARNOLD_LIBRARY
             NAMES ai
             PATHS ${ARNOLD_HOME}/bin
             DOC "Arnold library")

find_file(ARNOLD_KICK
          names kick
          PATHS ${ARNOLD_HOME}/bin
          DOC "Arnold kick executable")

find_file(ARNOLD_PYKICK
          names pykick
          PATHS ${ARNOLD_HOME}/python/pykikc
          DOC "Arnold pykick executable")

find_file(ARNOLD_MAKETX
          names maketx
          PATHS ${ARNOLD_HOME}/bin
          DOC "Arnold maketx executable")

find_path(ARNOLD_INCLUDE_DIR ai.h
          PATHS ${ARNOLD_HOME}/include
          DOC "Arnold include path")

find_path(ARNOLD_PYTHON_DIR arnold/ai_allocate.py
          PATHS ${ARNOLD_HOME}/python
          DOC "Arnold python bindings path")

find_file(ARNOLD_OSLC
          names oslc
          PATHS ${ARNOLD_HOME}/bin
          DOC "Arnold flavoured oslc")

find_path(ARNOLD_OSL_HEADER_DIR stdosl.h
          PATHS ${ARNOLD_HOME}/osl/include
          DOC "Arnold flavoured osl headers")

set(ARNOLD_LIBRARIES ${ARNOLD_LIBRARY})
set(ARNOLD_INCLUDE_DIRS ${ARNOLD_INCLUDE_DIR})
set(ARNOLD_PYTHON_DIRS ${ARNOLD_PYTHON_DIR})
set(ARNOLD_OSL_HEADER_DIRS ${ARNOLD_OSL_HEADER_DIR})

if(ARNOLD_INCLUDE_DIR AND EXISTS "${ARNOLD_INCLUDE_DIR}/ai_version.h")
    foreach(_arnold_comp ARCH_NUM MAJOR_NUM MINOR_NUM FIX)
        file(STRINGS
             ${ARNOLD_INCLUDE_DIR}/ai_version.h
             _arnold_tmp
             REGEX "#define AI_VERSION_${_arnold_comp} .*$")
        string(REGEX MATCHALL "[0-9]+" ARNOLD_VERSION_${_arnold_comp} ${_arnold_tmp})
    endforeach()
    set(ARNOLD_VERSION ${ARNOLD_VERSION_ARCH_NUM}.${ARNOLD_VERSION_MAJOR_NUM}.${ARNOLD_VERSION_MINOR_NUM}.${ARNOLD_VERSION_FIX})
endif()

function(arnold_compile_osl)
    set(_arnold_options QUIET VERBOSE INSTALL INSTALL_SOURCES)
    set(_arnold_one_value_args OSLC_FLAGS DESTINATION DESTINATION_SOURCES)
    set(_arnold_multi_value_args INCLUDES SOURCES)
    cmake_parse_arguments(arnold_compile_osl "${_arnold_options}" "${_arnold_one_value_args}" "${_arnold_multi_value_args}" ${ARGN})

    if (CMAKE_BUILD_TYPE MATCHES Debug)
        set(_arnold_oslc_opt_flags "-d -O0")
    elseif (CMAKE_BUILD_TYPE MATCHES Release)
        set(_arnold_oslc_opt_flags "-O2")
    elseif (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        set(_arnold_oslc_opt_flags "-d -O2")
    else ()
        set(_arnold_oslc_opt_flags "-O2")
    endif ()

    set(_arnold_oslc_flags "-I${ARNOLD_OSL_HEADER_DIR}")
    set(_arnold_oslc_flags "${_arnold_oslc_flags} ${arnold_compile_osl_OSLC_FLAGS}")
    if (${arnold_compile_osl_QUIET})
        set(_arnold_oslc_flags "${_arnold_oslc_flags} -q")
    endif ()

    if (${arnold_compile_osl_VERBOSE})
        set(_arnold_oslc_flags "${_arnold_oslc_flags} -v")
    endif ()

    foreach (_arnold_include ${arnold_compile_osl_INCLUDES})
        set(_arnold_oslc_flags "${_arnold_oslc_flags} -I${_arnold_include}")
    endforeach ()

    set(_arnold_oslc_flags "${_arnold_oslc_flags} ${_arnold_oslc_opt_flags}")
    if (${arnold_compile_osl_VERBOSE})
        message (STATUS "OSL - Arnold compile options : ${_arnold_oslc_flags}")
    endif ()

    foreach (_arnold_source ${arnold_compile_osl_SOURCES})
        # unique name for each target
        string(REPLACE ".osl" ".oso" _arnold_target_name ${_arnold_source})
        string(REPLACE "/" "_" _arnold_target_name ${_arnold_target_name})
        string(REPLACE "\\" "_" _arnold_target_name ${_arnold_target_name})
        string(REPLACE ":" "_" _arnold_target_name ${_arnold_target_name})
        set(_arnold_target_path "${CMAKE_CURRENT_BINARY_DIR}/${_arnold_target_name}")
        string(REPLACE "." "_" _arnold_target_name ${_arnold_target_name})
        set(_arnold_cmd_args "${_arnold_oslc_flags} -o ${_arnold_target_path} ${_arnold_source}")
        separate_arguments(_arnold_cmd_args)
        add_custom_command(OUTPUT ${_arnold_target_path}
                           COMMAND ${ARNOLD_OSLC} ${_arnold_cmd_args}
                           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_custom_target(${_arnold_target_name} ALL
                          DEPENDS ${_arnold_target_path}
                          SOURCES ${_arnold_source})
        if (${arnold_compile_osl_INSTALL})
            get_filename_component(_arnold_install_name ${_arnold_source} NAME)
            # rename the unique files
            string(REPLACE ".osl" ".oso" _arnold_install_name ${_arnold_install_name})
            install(FILES ${_arnold_target_path}
                    DESTINATION ${arnold_compile_osl_DESTINATION}
                    RENAME ${_arnold_install_name})
        endif ()
        if (${arnold_compile_osl_INSTALL_SOURCES})
            install(FILES ${_arnold_source} DESTINATION ${arnold_compile_osl_DESTINATION_SOURCES})
        endif ()
    endforeach ()
endfunction()

message(STATUS "Arnold library: ${ARNOLD_LIBRARY}")
message(STATUS "Arnold headers: ${ARNOLD_INCLUDE_DIR}")
message(STATUS "Arnold version: ${ARNOLD_VERSION}")

include(FindPackageHandleStandardArgs)

if (${ARNOLD_VERSION_ARCH_NUM} VERSION_GREATER "4")
    find_package_handle_standard_args(Arnold
                                      REQUIRED_VARS
                                      ARNOLD_LIBRARY
                                      ARNOLD_INCLUDE_DIR
                                      ARNOLD_OSLC
                                      ARNOLD_OSL_HEADER_DIR
                                      VERSION_VAR
                                      ARNOLD_VERSION)
else ()
    find_package_handle_standard_args(Arnold
                                      REQUIRED_VARS
                                      ARNOLD_LIBRARY
                                      ARNOLD_INCLUDE_DIR
                                      VERSION_VAR
                                      ARNOLD_VERSION)
endif ()
