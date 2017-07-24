set(APP_NAME partusdtopology)

file(GLOB SRC *.cpp)

add_executable(${APP_NAME} ${SRC})
target_link_libraries(${APP_NAME} ${PARTIO_LIBRARIES} ${PYTHON_LIBRARIES} ${Boost_LIBRARIES} ${MAYA_LIBS} arch tf gf sdf usd usdGeom)

install(TARGETS ${APP_NAME}
        DESTINATION bin)
