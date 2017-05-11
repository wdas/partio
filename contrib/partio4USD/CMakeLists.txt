add_compile_options(-Wall -std=c++11 -DBUILD_OPTLEVEL_OPT -DBUILD_COMPONENT_SRC_PREFIX="")

include_directories($ENV{USD_ROOT}/include)
link_directories($ENV{USD_ROOT}/lib)

add_subdirectory(format)

install(FILES plugInfo.json
        DESTINATION ./)