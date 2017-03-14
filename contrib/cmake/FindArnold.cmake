find_library(ARNOLD_LIBRARY
        NAMES ai
        PATHS $ENV{ARNOLD_HOME}/bin
        DOC "Arnold library")

find_file(ARNOLD_KICK
        names kick
        PATHS $ENV{ARNOLD_HOME}/bin
        DOC "Arnold kick executable")

find_file(ARNOLD_PYKICK
        names pykick
        PATHS $ENV{ARNOLD_HOME}/python/pykikc
        DOC "Arnold pykick executable")

find_file(ARNOLD_MAKETX
        names maketx
        PATHS $ENV{ARNOLD_HOME}/bin
        DOC "Arnold maketx executable")

find_path(ARNOLD_INCLUDE_DIR ai.h
        PATHS $ENV{ARNOLD_HOME}/include
        DOC "Arnold include path")

find_path(ARNOLD_PYTHON_DIR arnold/ai_allocate.py
        PATHS $ENV{ARNOLD_HOME}/python
        DOC "Arnold python bindings path")

if(ARNOLD_INCLUDE_DIR AND EXISTS "${ARNOLD_INCLUDE_DIR}/ai_version.h")
    foreach(comp ARCH_NUM MAJOR_NUM MINOR_NUM FIX)
        file(STRINGS
                ${ARNOLD_INCLUDE_DIR}/ai_version.h
                TMP
                REGEX "#define AI_VERSION_${comp} .*$")
        string(REGEX MATCHALL "[0-9]+" ARNOLD_VERSION_${comp} ${TMP})
    endforeach()
    set(ARNOLD_VERSION ${ARNOLD_VERSION_ARCH_NUM}.${ARNOLD_VERSION_MAJOR_NUM}.${ARNOLD_VERSION_MINOR_NUM}.${ARNOLD_VERSION_FIX})
endif()

message(STATUS "Arnold library: ${ARNOLD_LIBRARY}")
message(STATUS "Arnold headers: ${ARNOLD_INCLUDE_DIR}")
message(STATUS "Arnold version: ${ARNOLD_VERSION}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Arnold
        REQUIRED_VARS
        ARNOLD_LIBRARY
        ARNOLD_INCLUDE_DIR
        VERSION_VAR
        ARNOLD_VERSION)
