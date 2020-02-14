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

if(with_MPI)
  CMAKE_FORCE_C_COMPILER(mpifcc GNU)
  CMAKE_FORCE_CXX_COMPILER(mpiFCC GNU)
  CMAKE_FORCE_Fortran_COMPILER(mpifrt GNU)
else()
  CMAKE_FORCE_C_COMPILER(fcc GNU)
  CMAKE_FORCE_CXX_COMPILER(FCC GNU)
  CMAKE_FORCE_Fortran_COMPILER(frt GNU)
endif()

set(CMAKE_FIND_ROOT_PATH /opt/FJSVpclang/1.2.0)   # RIIT CX400
set(CMAKE_INCLUDE_PATH /opt/FJSVpclang/1.2.0/include)
set(CMAKE_LIBRARY_PATH /opt/FJSVpclang/1.2.0/lib64)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(TARGET_ARCH "ITO_TCS")
set(USE_F_TCS "YES")
