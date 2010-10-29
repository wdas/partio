include_directories(
   ${CMAKE_SOURCE_DIR}/src/lib
)

ADD_EXECUTABLE(partview partview.cpp)
target_link_libraries(partview ${PARTIO_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARY})

ADD_EXECUTABLE(partinfo partinfo.cpp)
target_link_libraries(partinfo ${PARTIO_LIBRARIES})

ADD_EXECUTABLE(partconv partconv.cpp)
target_link_libraries(partconv ${PARTIO_LIBRARIES})



