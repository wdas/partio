##### THIS WILL NEED SOME HELP WITH  NON-Linux compiles

## CMake compatibility issues: don't modify this, please!
cmake_minimum_required(VERSION 2.4.6)

if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)


if (UNIX AND NOT APPLE)
    add_definitions(-DLINUX)
    add_definitions(-Werror)
endif ()
if (APPLE )
    add_definitions(-DMAC_PLUGIN)
    add_definitions(-DOSMac_)
    add_definitions(-DOSMac_MachO)
endif ()

set(CMAKE_CXX_WARNING_LEVEL 4)


mark_as_advanced(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
## Use verbose make file
set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(PLUGIN_NAME "partio4Maya")
set(CMAKE_PROJECT_NAME "partio4Maya")

if (APPLE)
    set(PLUGIN_SUFFIX ".bundle")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".bundle")
    set(SUFFIX ".bundle")
endif ()

if (WIN32)
    set(PLUGIN_SUFFIX ".mll")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".mll")
    set(SUFFIX ".mll")
endif ()

if (UNIX)
    set(PLUGIN_SUFFIX ".so")
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
    set(SUFFIX ".so")
endif ()

# FIXME: why is this necessary?
set(STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
set(STD_LIB "/usr/lib/")

include_directories(${STD_INCLUDES} ${MAYA_INCLUDE_DIRS})
link_directories(${STD_LIB} ${MAYA_LIBRARY_DIR})

add_definitions(-D_BOOL)
add_definitions(-DREQUIRE_IOSTREAM)
if (!WIN32)
    add_definitions(-fPIC)
    add_definitions(-pthread)
endif ()

file(GLOB partio4MayaCPP "*.cpp")

if (APPLE)
    set(OPENGL_gl_LIBRARY "-framework OpenGL")
    set(OPENGL_glu_LIBRARY "-framework GLUT")
elseif (WIN32)
    set(OPENGL_gl_LIBRARY "openGL32")
    set(OPENGL_glu_LIBRARY "glu32")
else ()
    set(OPENGL_gl_LIBRARY "GL")
    set(OPENGL_glu_LIBRARY "GLU")
endif ()

set(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)


ADD_LIBRARY(${PLUGIN_NAME} SHARED ${partio4MayaCPP})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

## need these to build for windows or else the plugin won't load
if (WIN32)
    set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin")
endif ()

find_library(GLEW_STATIC_LIBRARY NAMES libGLEW.a)

TARGET_LINK_LIBRARIES(${PLUGIN_NAME}
                      ${GLEW_STATIC_LIBRARY}
                      ${ZLIB}
                      ${PARTIO_LIBRARIES}
                      ${MAYA_LIBS}
                      ${OPENGL_gl_LIBRARY}
                      ${OPENGL_glu_LIBRARY})

file(GLOB PARTIO4MAYA_SCRIPTS "scripts/*.mel")
file(GLOB PARTIO4MAYA_ICONS "icons/*")

set(MAYA_INST_DIR "maya/")
set(MAYA_PLUGIN_DIR "maya/plug-ins")

if (${LOCAL_INSTALL})
    set(MAYA_INST_DIR "maya/${MAYA_VERSION}/")
    set(MAYA_PLUGIN_DIR "maya/${MAYA_VERSION}/plug-ins/${PARTIO_BUILD_PLATFORM}")
else ()
    set(MAYA_INST_DIR "")
    set(MAYA_PLUGIN_DIR "plug-ins")
endif ()

install(TARGETS ${PLUGIN_NAME}
    DESTINATION ${MAYA_PLUGIN_DIR})

install(FILES ${PARTIO4MAYA_SCRIPTS}
    DESTINATION ${MAYA_INST_DIR}scripts)

install(FILES ${PARTIO4MAYA_ICONS}
    DESTINATION ${MAYA_INST_DIR}icons)
