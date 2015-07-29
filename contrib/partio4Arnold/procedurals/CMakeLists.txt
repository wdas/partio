##### THIS WILL NEED SOME HELP WITH  NON-Linux compiles

## CMake compatibility issues: don't modify this, please!
cmake_minimum_required(VERSION 2.4.6)

if(COMMAND cmake_policy)
     cmake_policy(SET CMP0003 NEW)
   cmake_policy(SET CMP0015 NEW)  # to supress relative library directory warnings
endif(COMMAND cmake_policy)

set(CMAKE_CXX_WARNING_LEVEL 4)

mark_as_advanced(CMAKE_BACKWARDS_COMPATIBILITY)
## allow more human readable "if then else" constructs
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
## Use verbose make file
set(CMAKE_VERBOSE_MAKEFILE TRUE)

set (PLUGIN_NAME "partioGenerator")
set (CMAKE_PROJECT_NAME "partioGenerator")

if (APPLE)
  set (PLUGIN_SUFFIX ".bundle")
  set (CMAKE_SHARED_LIBRARY_SUFFIX ".bundle")
  set (SUFFIX ".bundle")
endif()

if (WIN32)
  set (PLUGIN_SUFFIX ".dll")
  set (CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
  set (SUFFIX ".dll")
endif ()

if (UNIX)
  set (PLUGIN_SUFFIX ".so")
  set (CMAKE_SHARED_LIBRARY_SUFFIX ".so")
  set (SUFFIX ".so")
endif()

set (STD_INCLUDES "/usr/include" "/usr/include/CC")
set (STD_LIB "/usr/lib/")

include_directories(${STD_INCLUDES} ${ARNOLD_INCLUDE_DIR})
link_directories(${STD_LIB} ${ARNOLD_LIBRARY})
message(PARTIO_LIB_DIR)

if (WIN32)
  set(PARTIO_LIB partio)
elseif (APPLE)
  set(PARTIO_LIB libpartio.a)
elseif (UNIX)
  set(PARTIO_LIB libpartio.a)
endif ()

add_library(${PLUGIN_NAME} SHARED "partioGenerator.cpp")
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ./ )
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

target_link_libraries(${PLUGIN_NAME} ${PARTIO_LIBRARIES} ${ARNOLD_LIBRARY})

set(PARTIO4ARNOLD_OUTPUT_PATH "arnold/procedurals")

install(TARGETS ${PLUGIN_NAME}
         DESTINATION ${PARTIO4ARNOLD_OUTPUT_PATH})


