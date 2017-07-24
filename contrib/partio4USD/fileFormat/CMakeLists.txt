set(PLUGIN_NAME usdPartIO)

add_library(${PLUGIN_NAME} SHARED fileFormat.cpp)
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN_NAME} ${PARTIO_LIBRARIES} ${Boost_LIBRARIES} tf gf sdf usd usdGeom)

find_program(PARTINFO partinfo)
execute_process(COMMAND ${PARTINFO} --supportedReadFormats
                OUTPUT_VARIABLE PARTINFO_OUT)
string(REPLACE " " "\", \"" PARTINFO_OUT ${PARTINFO_OUT})
set(PLUGINFO_OUT ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json)
set(PARTIO_SUPPORTED_FORMAT_LIST "\"${PARTINFO_OUT}\"")
configure_file(plugInfo.json.in ${PLUGINFO_OUT})

install(TARGETS ${PLUGIN_NAME}
        DESTINATION .)

install(FILES ${PLUGINFO_OUT}
        DESTINATION ${PLUGIN_NAME}/resources/)