cmake_minimum_required(VERSION 2.4.6)

if (BUILD_PARTIO_MTOA)
    find_package(MtoA)
endif ()

if(MTOA_FOUND)
    add_subdirectory(plugin)
endif()

if (ARNOLD_VERSION_ARCH_NUM VERSION_GREATER "4")
    add_compile_options(-std=c++11)
endif ()

add_subdirectory(procedurals)
add_subdirectory(shaders)
