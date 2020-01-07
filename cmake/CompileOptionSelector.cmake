###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2020 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2020 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################

##
## Compile option selector
##

macro (AddOptimizeOption)
  if (TARGET_ARCH STREQUAL "FX10")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFX10 -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFX10 -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Cpp -DFX10 -Kfast -Nrt_notune -Knooptmsg")
    # -Xg   : gcc compatible flag to suppress -rdynamic
    if(enable_PreciseTimer)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Nfjcex")
    endif()

  elseif (TARGET_ARCH STREQUAL "FX100")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Cpp -Kfast -Nrt_notune -Knooptmsg")
    if(enable_PreciseTimer)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Nfjcex")
    endif()

  elseif (TARGET_ARCH STREQUAL "ITO_TCS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Cpp -Kfast -Nrt_notune -Knooptmsg")
    if(enable_PreciseTimer)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Nfjcex")
    endif()
    
  elseif (USE_F_TCS STREQUAL "YES")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Kfast -Nrt_notune -w -Xg")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Cpp -Kfast -Nrt_notune -Knooptmsg")
    if(enable_PreciseTimer)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Nfjcex")
    endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -cpp -O3 -Wall")

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -xHOST -O3 -qopt-report=3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -xHOST -O3 -qopt-report=3")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fpp -xHOST -O3 -qopt-report=3")

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fast -O3 -v")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fast -O3")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fast -O3 -v")
  else()
    message("using default option")
  endif()
endmacro()

macro (AddSSE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # nothing
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fastsse")
  endif()
endmacro()

macro (FreeForm)
  if(CMAKE_Fortran_COMPILER MATCHES ".*frtpx$")
    #set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS}")
    
  elseif (USE_F_TCS STREQUAL "YES")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Free")

  elseif(TARGET_ARCH STREQUAL "ITO_TCS")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Free")

  elseif(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -ffree-form --free-line-length-none")

  elseif(CMAKE_Fortran_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -free")

  elseif(CMAKE_Fortran_COMPILER_ID STREQUAL "PGI")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Mfree")

  endif()
endmacro()


macro(C99)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -c99")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
  endif()
endmacro()


macro(CPP11)
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
  CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  endif()
endmacro()


macro(checkOpenMP)
  if(enable_OPENMP)
    if(USE_F_TCS STREQUAL "YES")
      set(OpenMP_C_FLAGS "-Kopenmp")
      set(OpenMP_CXX_FLAGS "-Kopenmp")
      set(OpenMP_Fortran_FLAGS "-Kopenmp")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set(OpenMP_C_FLAGS "-fopenmp")
      set(OpenMP_CXX_FLAGS "-fopenmp")
      set(OpenMP_Fortran_FLAGS "-fopenmp")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
      set(OpenMP_C_FLAGS "-qopenmp")
      set(OpenMP_CXX_FLAGS "-qopenmp")
      set(OpenMP_Fortran_FLAGS "-qopenmp")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
      set(OpenMP_C_FLAGS "-mp")
      set(OpenMP_CXX_FLAGS "-mp")
      set(OpenMP_Fortran_FLAGS "-mp")
    else()
      find_package(OpenMP REQUIRED)
    endif()

    # OpenMP_*_FLAGSにはfind_package(OpenMP REQUIRED)でオプションフラグが設定される
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${OpenMP_Fortran_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
  endif()
endmacro()
