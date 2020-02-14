###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2020 RIKEN Center for Computational Science(R-CCS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2020 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################

# - Try to find PMlib
# Once done, this will define
#
#  PM_FOUND - system has PMlib
#  PM_INCLUDE_DIRS - PMlib include directories
#  PM_LIBRARIES - link these to use PMlib

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(PM_PKGCONF PM)

if(CMAKE_PREFIX_PATH)
  set(PM_CANDIDATE_PATH ${CMAKE_PREFIX_PATH})
  file(GLOB tmp "${CMAKE_PREFIX_PATH}/[Jj][Hh][Pp][Cc][Nn][Dd][Ff]*/")
  list(APPEND PM_CANDIDATE_PATH ${tmp})
endif()

# Include dir
find_path(PM_INCLUDE_DIR
  NAMES PerfMonitor.h
  PATHS ${PM_ROOT} ${PM_PKGCONF_INCLUDE_DIRS} ${PM_CANDIDATE_PATH}
  PATH_SUFFIXES include
)

# Finally the library itself
find_library(PM_LIBRARY
  NAMES PM
  PATHS ${PM_ROOT} ${PM_PKGCONF_LIBRARY_DIRS} ${PM_CANDIDATE_PATH}
  PATH_SUFFIXES lib
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(PM_PROCESS_INCLUDES PM_INCLUDE_DIR)
set(PM_PROCESS_LIBS PM_LIBRARY)
libfind_process(PM)
