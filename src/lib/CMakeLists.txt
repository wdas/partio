
FILE(GLOB io_cpp "io/*.cpp")
FILE(GLOB core_cpp "core/*.cpp")
ADD_LIBRARY (partio ${io_cpp} ${core_cpp})

FILE(GLOB public_includes "*.h")

INSTALL (TARGETS partio DESTINATION lib)
INSTALL (FILES ${public_includes} DESTINATION include)
