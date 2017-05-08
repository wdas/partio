set(PLUGIN_NAME usdPartIO)

find_package(Boost REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(TBB REQUIRED)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})
include_directories(SYSTEM ${TBB_INCLUDE_DIRS})

add_library(${PLUGIN_NAME} SHARED fileFormat.cpp)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN_NAME} ${PARTIO_LIBRARIES} tf gf sdf usd usdGeom)

install(TARGETS ${PLUGIN_NAME}
        DESTINATION .)

install(FILES plugInfo.json
        DESTINATION ${PLUGIN_NAME}/resources/)