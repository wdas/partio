# - Maya finder module
# This module searches for a valid Maya instalation.
# It searches for Maya's devkit, libraries, executables
# and related paths (scripts)
#
# Variables that will be defined:
# MAYA_FOUND             Defined if a Maya installation has been detected
# MAYA_EXECUTABLE        Path to Maya's executable
# MAYA_<lib>_FOUND       Defined if <lib> has been found
# MAYA_<lib>_LIBRARY     Path to <lib> library
# MAYA_LIBRARY_DIR       Path to the library directory
# MAYA_INCLUDE_DIRS      Path to the devkit's include directories
# MAYA_PLUGIN_SUFFIX     File extension for the maya plugin
# MAYA_QT_VERSION_SHORT  Version of Qt used by Maya (e.g. 4.7)
# MAYA_QT_VERSION_LONG   Full version of Qt used by Maya (e.g. 4.7.1)
#
# Macros provided:
# MAYA_SET_PLUGIN_PROPERITES  passed the target name, this sets up typical plugin
#                             properties like macro defines, prefixes, and suffixes
#
# Naming convention:
#  Local variables of the form _maya_foo
#  Input variables from CMake of the form Maya_FOO
#  Output variables of the form MAYA_FOO
#

#=============================================================================
# Macros
#=============================================================================

# macro for setting up typical plugin properties.
# this includes:
#   OS-specific plugin suffix (.mll, .so, .bundle)
#   Removal of 'lib' prefix on osx/linux
#   OS-specific defines
#   Post-commnad for correcting Qt library linking on osx
#   Windows link flags for exporting initializePlugin/uninitializePlugin
MACRO( MAYA_SET_PLUGIN_PROPERTIES target)
  SET_TARGET_PROPERTIES( ${target} PROPERTIES
    SUFFIX ${MAYA_PLUGIN_SUFFIX}
  )

  SET(_maya_DEFINES REQUIRE_IOSTREAM _BOOL MAC_PLUGIN OSMac_ OSMac_MachO)

  IF(APPLE)
    SET(_maya_DEFINES "${_maya_DEFINES}" MAC_PLUGIN OSMac_ OSMac_MachO)
    SET_TARGET_PROPERTIES( ${target} PROPERTIES
      PREFIX ""
      COMPILE_DEFINITIONS "${_maya_DEFINES}"
    )

    IF(QT_LIBRARIES)
      SET(_changes "")
      FOREACH(_lib ${QT_LIBRARIES})
        IF("${_lib}" MATCHES ".*framework.*")
          GET_FILENAME_COMPONENT(_shortname ${_lib} NAME)
          STRING(REPLACE ".framework" "" _shortname ${_shortname})
          # FIXME: QT_LIBRARIES does not provide the entire path to the lib.
          #  it provides /usr/local/qt/4.7.2/lib/QtGui.framework
          #  but we need /usr/local/qt/4.7.2/lib/QtGui.framework/Versions/4/QtGui
          # below is a hack, likely to break on other configurations
          SET(_changes ${_changes} "-change" "${_lib}/Versions/4/${_shortname}" "@executable_path/${_shortname}")
        ENDIF()
      ENDFOREACH(_lib)

      ADD_CUSTOM_COMMAND(
        TARGET ${target}
        POST_BUILD
        COMMAND install_name_tool ${_changes} $<TARGET_FILE:${target}>
      )
    ENDIF(QT_LIBRARIES)

  ELSEIF(WIN32)
    SET(_maya_DEFINES "${_maya_DEFINES}" _AFXDLL _MBCS NT_PLUGIN)
    SET_TARGET_PROPERTIES( ${target} PROPERTIES
      LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin"
      COMPILE_DEFINITIONS "${_maya_DEFINES}"
    )
  ELSE()
    SET(_maya_DEFINES "${_maya_DEFINES}" LINUX LINUX_64)
    SET_TARGET_PROPERTIES( ${target} PROPERTIES
      PREFIX ""
      COMPILE_DEFINITIONS "${_maya_DEFINES}"
    )
  ENDIF()

ENDMACRO(MAYA_SET_PLUGIN_PROPERTIES)


SET(_maya_TEST_VERSIONS)
SET(_maya_KNOWN_VERSIONS "2008" "2009" "2010" "2011" "2012" "2013")

IF(APPLE)
  SET(MAYA_PLUGIN_SUFFIX ".bundle")
ELSEIF(WIN32)
  SET(MAYA_PLUGIN_SUFFIX ".mll")
ELSE() #LINUX
  SET(MAYA_PLUGIN_SUFFIX ".so")
ENDIF()

# generate list of versions to test
IF(Maya_FIND_VERSION_EXACT)
  LIST(APPEND _maya_TEST_VERSIONS "${Maya_FIND_VERSION}")
ELSE(Maya_FIND_VERSION_EXACT)
  IF(Maya_FIND_VERSION)
    FOREACH(version ${_maya_KNOWN_VERSIONS})
      IF(NOT "${version}" VERSION_LESS "${Maya_FIND_VERSION}")
        LIST(APPEND _maya_TEST_VERSIONS "${version}" )
      ENDIF()
    ENDFOREACH(version)
  ELSE(Maya_FIND_VERSION)
    SET(_maya_TEST_VERSIONS ${_maya_KNOWN_VERSIONS})
  ENDIF(Maya_FIND_VERSION)
ENDIF(Maya_FIND_VERSION_EXACT)

SET(_maya_TEST_PATHS)

# generate list of paths to test
FOREACH(version ${_maya_TEST_VERSIONS})
  IF(APPLE)
    LIST(APPEND _maya_TEST_PATHS "/Applications/Autodesk/maya${version}/Maya.app/Contents")
  ELSEIF(WIN32)
    SET(_maya_TEST_PATHS ${_maya_TEST_PATHS}
      "$ENV{PROGRAMFILES}/Autodesk/Maya${version}-x64"
      "$ENV{PROGRAMFILES}/Autodesk/Maya${version}"
      "C:/Program Files/Autodesk/Maya${version}-x64"
      "C:/Program Files/Autodesk/Maya${version}"
      "C:/Program Files (x86)/Autodesk/Maya${version}"
   )
  ELSE() #Linux
    SET(_maya_TEST_PATHS ${_maya_TEST_PATHS}
      "/usr/autodesk/maya${version}-x64"
      "/usr/autodesk/maya${version}"
    )
  ENDIF()
ENDFOREACH(version)

# search for maya within the MAYA_LOCATION and PATH env vars and test paths
FIND_PROGRAM(MAYA_EXECUTABLE maya
  PATHS
    $ENV{MAYA_LOCATION}
    ${_maya_TEST_PATHS}
  PATH_SUFFIXES bin
  NO_SYSTEM_ENVIRONMENT_PATH
  DOC "Maya's executable path"
)

IF(MAYA_EXECUTABLE)
  # TODO: use GET_FILENAME_COMPONENT here
  STRING(REGEX REPLACE "/bin/maya.*" "" MAYA_LOCATION "${MAYA_EXECUTABLE}")

  STRING(REGEX MATCH "20[0-9][0-9]" MAYA_VERSION "${MAYA_LOCATION}")

  IF(Maya_FIND_VERSION)
    # test that we've found a valid version
    LIST(FIND _maya_TEST_VERSIONS ${MAYA_VERSION} _maya_FOUND_INDEX)
    IF(${_maya_FOUND_INDEX} EQUAL -1)
      MESSAGE(STATUS "Found Maya version ${MAYA_VERSION}, but requested at least ${Maya_FIND_VERSION}. Re-searching without environment variables...")
      SET(MAYA_LOCATION NOTFOUND)
      # search again, but don't use environment variables
      # (these should be only paths we constructed based on requested version)
      FIND_PATH(MAYA_LOCATION maya
        PATHS
          ${_maya_TEST_PATHS}
        PATH_SUFFIXES bin
        DOC "Maya's Base Directory"
        NO_SYSTEM_ENVIRONMENT_PATH
      )
      SET(MAYA_EXECUTABLE "${MAYA_LOCATION}/bin/maya" CACHE PATH "Maya's executable path")
      STRING(REGEX MATCH "20[0-9][0-9]" MAYA_VERSION "${MAYA_LOCATION}")
    #ELSE: error?
    ENDIF(${_maya_FOUND_INDEX} EQUAL -1)
  ENDIF(Maya_FIND_VERSION)
ENDIF(MAYA_EXECUTABLE)

# Qt Versions
IF(${MAYA_VERSION} STREQUAL "2011")
  SET(MAYA_QT_VERSION_SHORT CACHE STRING "4.5")
  SET(MAYA_QT_VERSION_LONG  CACHE STRING "4.5.3")
ELSEIF(${MAYA_VERSION} STREQUAL "2012")
  SET(MAYA_QT_VERSION_SHORT CACHE STRING "4.7")
  SET(MAYA_QT_VERSION_LONG  CACHE STRING "4.7.1")
ELSEIF(${MAYA_VERSION} STREQUAL "2013")
  SET(MAYA_QT_VERSION_SHORT CACHE STRING "4.7")
  SET(MAYA_QT_VERSION_LONG  CACHE STRING "4.7.1")
ENDIF()

# NOTE: the MAYA_LOCATION environment variable is often misunderstood.  On every OS it is expected to point
# directly above bin/maya. Relative paths from $MAYA_LOCATION to include, lib, and devkit directories vary depending on OS.

# We don't use environment variables below this point to lessen the risk of finding incompatible versions of
# libraries and includes (it could happen with highly non-standard configurations; i.e. maya libs in /usr/local/lib or on
# CMAKE_LIBRARY_PATH, CMAKE_INCLUDE_PATH, CMAKE_PREFIX_PATH).
# - If the maya executable is found in a standard location, or in $MAYA_LOCATION/bin or $PATH, and the
#   includes and libs are in standard locations relative to the binary, they will be found

MESSAGE(STATUS "Maya location: ${MAYA_LOCATION}")

FIND_PATH(MAYA_INCLUDE_DIR maya/MFn.h
  HINTS
    ${MAYA_LOCATION}
  PATH_SUFFIXES
    include               # linux and windows
    ../../devkit/include  # osx
  DOC "Maya's include path"
)

LIST(APPEND MAYA_INCLUDE_DIRS ${MAYA_INCLUDE_DIR})

FIND_PATH(MAYA_DEVKIT_INC_DIR GL/glext.h
  HINTS
    ${MAYA_LOCATION}
  PATH_SUFFIXES
	devkit/plug-ins/
  DOC "Maya's devkit headers path"
)
LIST(APPEND MAYA_INCLUDE_DIRS ${MAYA_DEVKIT_INC_DIR})


FIND_PATH(MAYA_LIBRARY_DIR libOpenMaya.dylib libOpenMaya.so OpenMaya.lib
  HINTS
    ${MAYA_LOCATION}
  PATH_SUFFIXES
    lib    # linux and windows
    MacOS  # osx
  DOC "Maya's library path"
)


FOREACH(_maya_lib
  OpenMaya
  OpenMayaAnim
  OpenMayaFX
  OpenMayaRender
  OpenMayaUI
  Image
  Foundation
  IMFbase
  tbb
#  cg
#  cgGL
)
  IF(APPLE)
    FIND_LIBRARY(MAYA_${_maya_lib}_LIBRARY ${_maya_lib}
      HINTS
        ${MAYA_LOCATION}
      PATHS
        ${_maya_TEST_PATHS}
      PATH_SUFFIXES
        MacOS  # osx
      NO_CMAKE_SYSTEM_PATH # this must be used or else Foundation.framework will be found instead of libFoundation
      DOC "Maya's ${MAYA_LIB} library path"
    )
  ELSE(APPLE)
    FIND_LIBRARY(MAYA_${_maya_lib}_LIBRARY ${_maya_lib}
      HINTS
        ${MAYA_LOCATION}
      PATHS
        ${_maya_TEST_PATHS}
      PATH_SUFFIXES
        lib    # linux and windows
      DOC "Maya's ${MAYA_LIB} library path"
    )
  ENDIF()
  LIST(APPEND MAYA_LIBRARIES ${MAYA_${_maya_lib}_LIBRARY})
ENDFOREACH(_maya_lib)


FIND_PATH(MAYA_USER_DIR
  NAMES
    ${MAYA_VERSION}-x64 ${MAYA_VERSION}
  PATHS
    $ENV{HOME}/Library/Preferences/Autodesk/maya  # osx
    $ENV{USERPROFILE}/Documents/maya              # windows
    $ENV{HOME}/maya                               # linux
  DOC "Maya user home directory"
  NO_SYSTEM_ENVIRONMENT_PATH
)

# IF (Maya_FOUND)
#     IF (NOT Maya_FIND_QUIETLY)
#       MESSAGE(STATUS "Maya version: ${Maya_MAJOR_VERSION}.${Maya_MINOR_VERSION}.${Maya_SUBMINOR_VERSION}")
#     ENDIF(NOT Maya_FIND_QUIETLY)
#     IF (NOT Maya_FIND_QUIETLY)
#       MESSAGE(STATUS "Found the following Maya libraries:")
#     ENDIF(NOT Maya_FIND_QUIETLY)
#     FOREACH ( COMPONENT  ${Maya_FIND_COMPONENTS} )
#       STRING( TOUPPER ${COMPONENT} UPPERCOMPONENT )
#       IF ( Maya_${UPPERCOMPONENT}_FOUND )
#         IF (NOT Maya_FIND_QUIETLY)
#           MESSAGE (STATUS "  ${COMPONENT}")
#         ENDIF(NOT Maya_FIND_QUIETLY)
#         SET(Maya_LIBRARIES ${Maya_LIBRARIES} ${Maya_${UPPERCOMPONENT}_LIBRARY})
#       ENDIF ( Maya_${UPPERCOMPONENT}_FOUND )
#     ENDFOREACH(COMPONENT)
# ELSE (Maya_FOUND)
#     IF (Maya_FIND_REQUIRED)
#       message(SEND_ERROR "Unable to find the requested Maya libraries.\n${Maya_ERROR_REASON}")
#     ENDIF(Maya_FIND_REQUIRED)
# ENDIF(Maya_FOUND)

# handle the QUIETLY and REQUIRED arguments and SET MAYA_FOUND to TRUE if
# all LISTed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Maya DEFAULT_MSG
  MAYA_LIBRARIES MAYA_EXECUTABLE MAYA_INCLUDE_DIR MAYA_LIBRARY_DIR MAYA_VERSION MAYA_PLUGIN_SUFFIX MAYA_USER_DIR
)

#
# Copyright 2012, Chad Dombrova
#
# vfxcmake is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# vfxcmake is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with vfxcmake.  If not, see <http://www.gnu.org/licenses/>.
#
