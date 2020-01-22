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


set(CMAKE_SYSTEM_NAME Linux)

include(CMakeForceCompiler)

# CMAKE_FORCE_Fortran_Compiler is not supported ver. 2.6
# A : cmake version >= 2.7
# B : cmake version < 2.7

if (CMAKE_MAJOR_VERSION GREATER 2)
  set(build_rule "A")
else()
  if(CMAKE_MINOR_VERSION GREATER 6)
    set(build_rule "A")
  else()
    set(build_rule "B")
  endif()
endif()


if(with_MPI)
  CMAKE_FORCE_C_COMPILER(mpifcc GNU)
  CMAKE_FORCE_CXX_COMPILER(mpiFCC GNU)
  if (build_rule STREQUAL "A")
    CMAKE_FORCE_Fortran_COMPILER(mpifrt GNU)
  else()
    set(CMAKE_Fortran_COMPILER mpifrt)
    set(CMAKE_Fortran_COMPILER_WORKS true)
  endif()
else()
  CMAKE_FORCE_C_COMPILER(fcc GNU)
  CMAKE_FORCE_CXX_COMPILER(FCC GNU)
  if (build_rule STREQUAL "A")
    CMAKE_FORCE_Fortran_COMPILER(frt GNU)
  else()
    set(CMAKE_Fortran_COMPILER frt)
    set(CMAKE_Fortran_COMPILER_WORKS true)
  endif()
endif()

set(CMAKE_FIND_ROOT_PATH /opt/FJSVtclang/1.2.0)
set(CMAKE_INCLUDE_PATH /opt/FJSVpclang/1.2.0/include)
set(CMAKE_LIBRARY_PATH /opt/FJSVpclang/1.2.0/lib64)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(TARGET_ARCH "F_TCS")
set(USE_F_TCS "YES")
