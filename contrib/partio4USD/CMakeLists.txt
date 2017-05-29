add_compile_options(-Wall -std=c++11 -DBUILD_OPTLEVEL_OPT -DBUILD_COMPONENT_SRC_PREFIX="")

include_directories($ENV{USD_ROOT}/include)
link_directories($ENV{USD_ROOT}/lib)

if (BUILD_PARTIO_USD)
    add_subdirectory(fileFormat)
endif ()

if (BUILD_PARTIO_USD_MAYA)
    add_subdirectory(primWriter)
else ()
    install(FILES plugInfo.json
            DESTINATION ./)
endif ()
