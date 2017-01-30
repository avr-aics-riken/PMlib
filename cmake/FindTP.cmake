###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2017 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2017 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################

# - Try to find TextParser
# Once done, this will define
#
#  TP_FOUND - system has TextParser
#  TP_INCLUDE_DIRS - TextParser include directories
#  TP_LIBRARIES - link these to use TextParser

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(TP_PKGCONF TP)

if(CMAKE_PREFIX_PATH)
  set(TP_CANDIDATE_PATH ${CMAKE_PREFIX_PATH})
  file(GLOB tmp "${CMAKE_PREFIX_PATH}/[Jj][Hh][Pp][Cc][Nn][Dd][Ff]*/")
  list(APPEND TP_CANDIDATE_PATH ${tmp})
endif()

# Include dir
find_path(TP_INCLUDE_DIR
  NAMES TextParser.h
  PATHS ${TP_ROOT} ${TP_PKGCONF_INCLUDE_DIRS} ${TP_CANDIDATE_PATH}
  PATH_SUFFIXES include
)

# Finally the library itself
find_library(TP_LIBRARY
  NAMES TP
  PATHS ${TP_ROOT} ${TP_PKGCONF_LIBRARY_DIRS} ${TP_CANDIDATE_PATH}
  PATH_SUFFIXES lib
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(TP_PROCESS_INCLUDES TP_INCLUDE_DIR)
set(TP_PROCESS_LIBS TP_LIBRARY)
libfind_process(TP)
