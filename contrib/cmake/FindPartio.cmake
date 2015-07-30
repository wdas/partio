# - PartIO finder module
# This module searches for a valid PartIO build.

find_path(PARTIO_LIBRARY_DIR libpartio.a
    PATHS $ENV{PARTIO_HOME}/lib
    DOC "PartIO library path")

find_path(PARTIO_INCLUDE_DIR Partio.h
    PATHS $ENV{PARTIO_HOME}/include
    DOC "PartIO include path")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Partio DEFAULT_MSG
    PARTIO_LIBRARY_DIR PARTIO_INCLUDE_DIR)
