SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Arnold)
find_package(Maya)
find_package(Houdini)

if(MAYA_FOUND)
	ADD_SUBDIRECTORY(partio4Maya)
endif()

if(NOT WIN32 AND NOT APPLE)
	if(ARNOLD_FOUND)
		ADD_SUBDIRECTORY(partio4Arnold)
	endif()

	if(HOUDINI_FOUND)
		ADD_SUBDIRECTORY(partio4Houdini)
	endif()
endif(NOT WIN32 AND NOT APPLE)
