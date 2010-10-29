ADD_EXECUTABLE(makeline makeline.cpp)
target_link_libraries(makeline ${PARTIO_LIBRARIES})

ADD_EXECUTABLE(makecircle makecircle.cpp)
target_link_libraries(makecircle ${PARTIO_LIBRARIES})

ADD_EXECUTABLE(testcache testcache.cpp)
target_link_libraries(testcache ${PARTIO_LIBRARIES})

ADD_EXECUTABLE(test test.cpp)
target_link_libraries(test ${PARTIO_LIBRARIES})

ADD_EXECUTABLE(testiterator testiterator.cpp)
target_link_libraries(testiterator ${PARTIO_LIBRARIES})
