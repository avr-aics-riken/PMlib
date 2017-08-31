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

set(CMAKE_SYSTEM_NAME Linux)

include(CMakeForceCompiler)

if(with_MPI)
  CMAKE_FORCE_C_COMPILER(mpifccpx GNU)
  CMAKE_FORCE_CXX_COMPILER(mpiFCCpx GNU)
  CMAKE_FORCE_Fortran_COMPILER(mpifrtpx GNU)
else()
  CMAKE_FORCE_C_COMPILER(fccpx GNU)
  CMAKE_FORCE_CXX_COMPILER(FCCpx GNU)
  CMAKE_FORCE_Fortran_COMPILER(frtpx GNU)
endif()

set(CMAKE_FIND_ROOT_PATH /opt/FJSVmxlang/GM-2.0.0-05)
set(CMAKE_INCLUDE_PATH /opt/FJSVmxlang/GM-2.0.0-05/include)
set(CMAKE_LIBRARY_PATH /opt/FJSVmxlang/GM-2.0.0-05/lib64)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(TARGET_ARCH "FX100")
set(USE_F_TCS "YES")
