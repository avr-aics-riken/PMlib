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
#
#-*- CMAKE -*-#
# Process this file with cmake to produce a configure script.

# Following parameters must be given at cmake command line

# Build options, see Readme.md for the details.
#
# -D INSTALL_DIR=install_directory
#
# -D with_example={no|yes}
#
# -D enable_Fortran={no|yes}
#
# -D with_MPI={no|yes}
#
# -D enable_OPENMP={no|yes}
#
# -D with_PAPI={no|yes|installed_directory}
#
# -D with_OTF={no|installed_directory}
#

cmake_minimum_required(VERSION 2.6)


#######
# Build options
#######

option (with_example "Compiling examples" "OFF")
option (enable_Fortran "Using Fortran" "OFF")
option (with_MPI "Enable MPI" "OFF")
option (enable_OPENMP "Enable OpenMP" "OFF")
option (with_PAPI "Enable PAPI" "OFF")
option (with_OTF "Enable tracing" "OFF")

#######
# Project setting
#######

if(enable_Fortran)
  message("PROJECT: PMlib for C, C++, and Fortran")
  project(PMlib C CXX Fortran)
else()
  message("PROJECT: PMlib for C and C++")
  project(PMlib C CXX)
endif()

if(INSTALL_DIR STREQUAL "")
  set(INSTALL_DIR "/usr/local/PMlib")
endif()

# project()でCMAKE_INSTALL_PREFIX=/usr/localとなるので、再定義
set (CMAKE_INSTALL_PREFIX "${INSTALL_DIR}")

#set(CMAKE_BUILD_TYPE Release)



#######
set(PROJECT_VERSION "5.6.1")
set(LIB_REVISION "20170330_0900")
#######




#######
# Cmake module
#######

set(CMAKE_MODULE_PATH  ${PROJECT_SOURCE_DIR}/cmake)
include(${PROJECT_SOURCE_DIR}/cmake/CompileOptionSelector.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/pm_package.cmake)



#######
# Options
#######

if(enable_Fortran)
  FreeForm()
endif()

AddOptimizeOption()

checkOpenMP()

if(NOT with_MPI)
  SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -DDISABLE_MPI")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_MPI")
endif()


#######
# Find libraries to depend
#######


#######
# Display options
#######

message(" ")
message( STATUS "Destination PATH: "               ${CMAKE_INSTALL_PREFIX})
message( STATUS "CMAKE_C_COMPILER: "               ${CMAKE_C_COMPILER})
message( STATUS "CMAKE_C_FLAGS: "                  ${CMAKE_C_FLAGS})
message( STATUS "CMAKE_CXX_COMPILER: "             ${CMAKE_CXX_COMPILER})
message( STATUS "CMAKE_CXX_FLAGS: "                ${CMAKE_CXX_FLAGS})

if(enable_Fortran)
  message( STATUS "CMAKE_Fortran_COMPILER: "         ${CMAKE_Fortran_COMPILER})
  message( STATUS "CMAKE_Fortran_FLAGS: "            ${CMAKE_Fortran_FLAGS})
endif()

message(" ")
message( STATUS "OpenMP: "                         ${enable_OPENMP})
message( STATUS "MPI: "                            ${with_MPI})
message( STATUS "PAPI: "                           ${with_PAPI})
message( STATUS "OTF: "                            ${with_OTF})
message( STATUS "Example: "                        ${with_example})
message(" ")

if(CMAKE_CXX_COMPILER MATCHES ".*FCCpx$")
else()
  if(with_MPI)
    include(FindMPI)

    if(MPI_CXX_FOUND)
      message("MPI-CXX-CMP  : ${MPI_CXX_COMPILER}")
      message("MPI-CXX-FLAGS: ${MPI_CXX_COMPILE_FLAGS}")
      message("MPI-CXX-INC  : ${MPI_CXX_INCLUDE_PATH}")
      message("MPI-LNK-FLAGS: ${MPI_CXX_LINK_FLAGS}")
      message("MPI-LIBS     : ${MPI_CXX_LIBRARIES}")
    endif(MPI_CXX_FOUND)
  endif()
endif()

message("${PROJECT_NAME} Version : ${PROJECT_VERSION}")



#######
# include PATH for libs
#######

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    )


#######
# Example
#######

if (with_example)
  enable_testing()
  message("Code test will be performed")
else()
  message("Code test is skipped")
endif()


add_definitions(-DHAVE_CONFIG_H)


#######
# PAPI
#######

if(NOT with_PAPI)
elseif(with_PAPI STREQUAL "yes")
  add_definitions(-DUSE_PAPI)
  set(OPT_PAPI "ON")
else()
  add_definitions(-DUSE_PAPI)
  set(OPT_PAPI "ON")
  set(PAPI_DIR "${with_PAPI}")
endif()


#######
# OTF
#######

if(NOT with_OTF)
elseif(with_OTF STREQUAL "yes")
  add_definitions(-DUSE_OTF)
  set(OPT_OTF "ON")
else()
  add_definitions(-DUSE_OTF)
  set(OPT_OTF "ON")
  set(OTF_DIR "${with_OTF}")
endif()


#######
# Check header files
#######

include(CheckIncludeFiles)

CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)


#######
# Sub Directories
#######

add_subdirectory(src)
add_subdirectory(doc)

if(OPT_PAPI)
  add_subdirectory(src_papi_ext)
endif()

if(OPT_OTF)
  add_subdirectory(src_otf_ext)
endif()

if(with_example)
  add_subdirectory(example)
endif()


#######
# configure files
#######

configure_file( config.h.in.cmake config.h )
configure_file( include/pmVersion.h.in ${PROJECT_BINARY_DIR}/include/pmVersion.h @ONLY)


#######
# Install files
#######

install(FILES  ChangeLog.md
               Readme.md
               License.txt
               ${PROJECT_BINARY_DIR}/CMakeCache.txt
        DESTINATION share)


#######
# Installer
#######

pm_package()
