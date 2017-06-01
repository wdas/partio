set(PLUGIN_NAME partioVisualizerWriter)

include_directories($ENV{USD_ROOT}/third_party/maya/include)
link_directories($ENV{USD_ROOT}/third_party/maya/lib)

include_directories(${MAYA_INCLUDE_DIRS})
link_directories(${MAYA_LIBRARY_DIR})

set(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)

find_package(Boost COMPONENTS regex REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(TBB REQUIRED)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})
include_directories(SYSTEM ${TBB_INCLUDE_DIRS})

file(GLOB SRC *.cpp)

add_library(${PLUGIN_NAME} SHARED ${SRC})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN_NAME} ${PARTIO_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${MAYA_LIBS} tf gf sdf usd usdGeom usdMaya)

install(TARGETS ${PLUGIN_NAME}
        DESTINATION plug-ins)

set(PLUGINFO_OUT ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json)
configure_file(plugInfo.json.in ${PLUGINFO_OUT})

install(FILES ${PLUGINFO_OUT}
        DESTINATION .)