##### THIS WILL NEED SOME HELP WITH  NON-Linux compiles

## CMake compatibility issues: don't modify this, please!
CMAKE_MINIMUM_REQUIRED( VERSION 2.4.6 )

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
endif(COMMAND cmake_policy)

set(CMAKE_CXX_WARNING_LEVEL 4)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)


mark_as_advanced(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
## Use verbose make file
set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(PLUGIN_NAME "partioShaders")
set(CMAKE_PROJECT_NAME "partioShaders")

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

set(PARTIO_INCLUDE ${INCLUDE_OUTPUT_PATH})
set(PARTIO_LIBDIR  ${LIBRARY_OUTPUT_PATH})

file(GLOB partio4ArnoldCPP "*.cpp")
file(GLOB partio4ArnoldINCLUDES "*.h")

include_directories(${STD_INCLUDES} ${partio4ArnoldINCLUDES} ${ARNOLD_INCLUDE_DIR}  ${PARTIO_INCLUDE})
link_directories(${STD_LIB}  ${ARNOLD_LIBRARY}  ${PARTIO_LIBDIR} ${ZLIB_LIBRARY_DIR})

set(TARGET_H ${partio4ArnoldINCLUDES})
set(TARGET_SRC ${partio4ArnoldCPP})

if (WIN32)
    set(PARTIO_LIB partio)
elseif (APPLE)
    set(PARTIO_LIB libpartio.a)
elseif (UNIX)
    set(PARTIO_LIB libpartio.a)
endif ()


add_library(${PLUGIN_NAME} SHARED ${TARGET_H} ${TARGET_SRC})
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ./)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PLUGIN_NAME} ${ZLIB} ${PARTIO_LIB} ${ARNOLD_LIBRARY} ${MtoA_mtoa_api_LIBRARY})

set(PARTIO4ARNOLDSHADE_OUTPUT_PATH "arnold/shaders")
if (BUILD_PARTIO_ARNOLD)
    set(PARTIO4ARNOLDSHADE_OUTPUT_PATH "shaders")
endif ()

install(TARGETS ${PLUGIN_NAME} DESTINATION ${PARTIO4ARNOLDSHADE_OUTPUT_PATH} )
file(GLOB PARTIO4ARNOLDSHADE_SCRIPTS "*.mel")
install(FILES ${PARTIO4ARNOLDSHADE_SCRIPTS} DESTINATION ${PARTIO4ARNOLDSHADE_OUTPUT_PATH})
