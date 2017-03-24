##### THIS WILL NEED SOME HELP WITH  NON-Linux compiles

## CMake compatibility issues: don't modify this, please!
cmake_minimum_required(VERSION 2.4.6)

# Check if the GCC version matches the required version on Linux
if (DEFINED USE_MAYA_CERTifIED_COMPILER AND USE_MAYA_CERTifIED_COMPILER)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        if (CMAKE_COMPILER_IS_GNUCC)
            execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
            message("GCC_VERSION is ${GCC_VERSION}")
            string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
            list(GET GCC_VERSION_COMPONENTS 0 GCC_VERSION_MAJOR)
            list(GET GCC_VERSION_COMPONENTS 1 GCC_VERSION_MINOR)
            list(GET GCC_VERSION_COMPONENTS 2 GCC_VERSION_PATCH)
            if (${GCC_VERSION_MAJOR} MATCHES "4"
                AND
                ${GCC_VERSION_MINOR} MATCHES "1"
                AND
                ${GCC_VERSION_PATCH} MATCHES "2"
                )
                message("CORRECT VERSION OF COMPILER FOR MAYA PLUGINS")
            else ()
            message(FATAL_ERROR "INCORRECT VERSION OF COMPILER FOR MAYA PLUGINS, EXPECTING GCC 4.1.2")
            endif ()
        endif ()
    endif ()
endif ()


if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
endif ()

set(CMAKE_CXX_WARNING_LEVEL 4)

mark_as_advanced(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
## Use verbose make file
set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(PLUGIN_NAME "partioTranslator")
set(CMAKE_PROJECT_NAME "partioVizTranslator")

if (APPLE)
    set(PLUGIN_SUFFIX ".bundle")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".bundle")
    set(SUFFIX ".bundle")
endif ()

if (WIN32)
    set(PLUGIN_SUFFIX ".dll")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
    set(SUFFIX ".dll")
endif ()

if (UNIX)
    set(PLUGIN_SUFFIX ".so")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
    set(SUFFIX ".so")
endif ()


set(STD_INCLUDES "/usr/include" "/usr/include/CC")
set(STD_LIB "/usr/lib/")

include_directories(${STD_INCLUDES}
                    ${MAYA_INCLUDE_DIR}
                    ${ARNOLD_INCLUDE_DIR}
                    ${MTOA_INCLUDE_DIR})
link_directories(${STD_LIB}
                 ${MAYA_LIBRARY_DIR}
                 ${MTOA_mtoa_api_LIBRARY}
                 ${ARNOLD_LIBRARY}
                 ${ZLIB_LIBRARY_DIR})

# Maya CFLAGS
if (APPLE)
    set(DEVKIT_LOCATION ${MAYA_LOCATION}/../../devkit)
    add_definitions(
        -DMAC_PLUGIN
        -D_BOOL
        -DCC_GNU_
        -DOSMac_
        -DOSMacOSX_
        -DREQUIRE_IOSTREAM
        -DOSMac_MachO_
        -D_LANGUAGE_C_PLUS_PLUS
        -include "${DEVKIT_LOCATION}/include/maya/OpenMayaMac.h"
        -fno-gnu-keywords
        -fpascal-strings
        -D_DARWIN # required by MtoA
    )
else ()
    add_definitions(
        -DBits64_
        -m64
        -DUNIX
        -D_BOOL
        -DLINUX
        -DFUNCPROTO
        -D_GNU_SOURCE
        -DLINUX_64
        -fPIC
        -fno-strict-aliasing
        -DREQUIRE_IOSTREAM
        -Wno-deprecated
        -Wall
        -Wno-multichar
        -Wno-comment
        -Wno-sign-compare
        -funsigned-char
        -Wno-reorder
        -fno-gnu-keywords
        -ftemplate-depth-25
        -pthread
        -D_LINUX # required by MtoA
    )
endif ()

# no need to explicitly include header files, as they are
# included using ""
file(GLOB partio4ArnoldCPP "*.cpp")

set(MAYA_LIBS
    ${MAYA_OpenMaya_LIBRARY}
    ${MAYA_Foundation_LIBRARY}
    ${MAYA_OpenMayaAnim_LIBRARY}
    ${MAYA_OpenMayaFX_LIBRARY}
    ${MAYA_OpenMayaUI_LIBRARY}
    ${MAYA_OpenMayaRender_LIBRARY})

add_library(${PLUGIN_NAME} SHARED ${partio4ArnoldCPP})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PLUGIN_NAME}
                      ${MAYA_LIBS}
                      ${PARTIO_LIBRARIES}
                      ${ARNOLD_LIBRARY}
                      ${MTOA_mtoa_api_LIBRARY})
if ("${MTOA_ARCH_VERSION_NUM}" MATCHES "1")
    if ("${MTOA_MAJOR_VERSION_NUM}" MATCHES "2")
        target_compile_options(${PLUGIN_NAME} PRIVATE -DMTOA12)
    elseif ("${MTOA_MAJOR_VERSION_NUM}" MATCHES "4")
        target_compile_options(${PLUGIN_NAME} PRIVATE -DMTOA14)
    else ()
        message(FATAL_ERROR "Unsupported MtoA 1.x version: ${MTOA_VERSION)")
    endif ()
endif ()

set(PARTIO4ARNOLD_OUTPUT_PATH "arnold/mtoa/extensions")
if (BUILD_PARTIO_MTOA)
    set (PARTIO4ARNOLD_OUTPUT_PATH "extensions")
elseif (${LOCAL_install}) # TODO : ask Johnc if this is still needed
    set (PARTIO4ARNOLD_OUTPUT_PATH "arnold/mtoa-$ENV{MTOA_VERSION}/${PARTIO_BUILD_PLATFORM}/extensions")
endif ()

install(TARGETS ${PLUGIN_NAME}
        DESTINATION ${PARTIO4ARNOLD_OUTPUT_PATH})

file(GLOB PARTIO4ARNOLD_SCRIPTS "*.py")
install(FILES ${PARTIO4ARNOLD_SCRIPTS}
        DESTINATION ${PARTIO4ARNOLD_OUTPUT_PATH})

