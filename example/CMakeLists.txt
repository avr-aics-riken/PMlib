##################################################################################
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

message(" ")
message("Examples : ")
message(STATUS "PAPI_DIR            = " ${PAPI_DIR})
message(STATUS "POWER_DIR            = " ${POWER_DIR})
message(STATUS "OTF_DIR             = " ${OTF_DIR})
message(STATUS "with_MPI            = " ${with_MPI})

#message(STATUS "PROJECT_BINARY_DIR = " ${PROJECT_BINARY_DIR})
#message(STATUS "PROJECT_SOURCE_DIR = " ${PROJECT_SOURCE_DIR})
#message(STATUS "IMPLICIT_LINK_LIBS = " ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
#message(STATUS "CFLAGS              = " ${CMAKE_C_FLAGS})
#message(STATUS "CXXFLAGS            = " ${CMAKE_CXX_FLAGS})
#message(STATUS "FortranFLAGS        = " ${CMAKE_Fortran_FLAGS})

include_directories(${PROJECT_BINARY_DIR}/include ${PROJECT_SOURCE_DIR}/include)

link_directories(${PROJECT_BINARY_DIR}/src)

if(OPT_PAPI)
  include_directories(${PAPI_DIR}/include)
  link_directories(${PROJECT_BINARY_DIR}/src_papi_ext)
  link_directories(${PAPI_DIR}/lib)
  #	link_directories(${PAPI_DIR}/lib64)
endif()

if(OPT_POWER)
  # Power API option is available on supercomputer Fugaku only
  include_directories(${POWER_DIR}/include)
  link_directories(${PROJECT_BINARY_DIR}/src_power_ext)
  link_directories(${POWER_DIR}/lib64)
endif()

if(OPT_OTF)
  include_directories(${OTF_DIR}/include/open-trace-format)
  link_directories(${PROJECT_BINARY_DIR}/src_otf_ext)
  link_directories(${OTF_DIR}/lib)
endif()


### Example programs
### Test 1, 2, 3 can be built for both serial program and MPI program.
### Test 4 and 5 are only for MPI environment.

#### Test1 : C++

add_executable(example1 ./test1/main_pmlib.cpp)

if(with_MPI)
  target_link_libraries(example1 -lPMmpi)
else()
  target_link_libraries(example1 -lPM)
endif()

if(OPT_PAPI)
  if(TARGET_ARCH STREQUAL "FUGAKU")
    #	target_link_libraries(example1 -Wl,'-lpapi_ext,-lpapi,-lpfm' -Nnofjprof)
    target_link_libraries(example1 -lpapi_ext -lpapi -lpfm -Nnofjprof)
  elseif(TARGET_ARCH STREQUAL "FX100")
  # on AICS FX100 static PAPI libs do not match the other system libs, i.e. bonehead system bug.
  # so the following dynamic libs must be used.
    target_link_libraries(example1 -lpapi_ext -Wl,'-lpapi,-lpfm')
  else()
  ## GNU linker ld options are better passed using -Wl, rather than -B compiler phase option
    target_link_libraries(example1 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
  endif()
endif()

if(OPT_POWER)
  if(TARGET_ARCH STREQUAL "FUGAKU")
    target_link_libraries(example1 -lpower_ext -lpwr )
  endif()
endif()

if(OPT_OTF)
  target_link_libraries(example1 -lotf_ext -lopen-trace-format)
endif()

if(with_MPI)
  set (test_parameters -np 2 "example1")
  add_test(NAME TEST_1 COMMAND "mpirun" ${test_parameters})
else()
  add_test(TEST_1 example1)
endif()



### Test 2 : Fortran

if(enable_Fortran)
  add_executable(example2 ./test2/f_main.f90)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  elseif(CMAKE_Fortran_COMPILER_ID STREQUAL "PGI")
    set_target_properties(example2 PROPERTIES LINKER_LANGUAGE Fortran)
    target_link_libraries(example2 -pgf90libs)
  else()
    set_target_properties(example2 PROPERTIES LINKER_LANGUAGE CXX)
  endif()

  if(with_MPI)
    target_link_libraries(example2 -lPMmpi)
  else()
    target_link_libraries(example2 -lPM)
  endif()

  if(OPT_PAPI)
    if(TARGET_ARCH STREQUAL "FUGAKU")
      target_link_libraries(example2 -lpapi_ext -lpapi -lpfm -Nnofjprof)
    elseif(TARGET_ARCH STREQUAL "FX100")
      target_link_libraries(example2 -lpapi_ext -Wl,'-lpapi,-lpfm')
    else()
      target_link_libraries(example2 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
    endif()
  endif()

  if(OPT_POWER)
    if(TARGET_ARCH STREQUAL "FUGAKU")
      target_link_libraries(example2 -lpower_ext -lpwr )
    endif()
  endif()

  if(OPT_OTF)
    target_link_libraries(example2 -lotf_ext -lopen-trace-format)
  endif()

  # Fujitsuの場合　--linkfortranは必須
  if(USE_F_TCS STREQUAL "YES")
    target_link_libraries(example2 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES} "--linkfortran")
  else()
#   target_link_libraries(example2 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
    target_link_libraries(example2 -lstdc++ )
  endif()

  if(with_MPI)
    set (test_parameters -np 2 "example2")
    add_test(NAME TEST_2 COMMAND "mpirun" ${test_parameters})
  else()
    add_test(TEST_2 example2)
  endif()

  if(TARGET_ARCH STREQUAL "FUGAKU")
    target_link_libraries(example2 --linkfortran )
  endif()

endif()



### Test3 : C

add_executable(example3
               ./test3/main_kernel.c
               ./test3/sub_kernel.c
)

if(with_MPI)
  target_link_libraries(example3 -lPMmpi)
else()
  target_link_libraries(example3 -lPM)
endif()

if(OPT_PAPI)
  if(TARGET_ARCH STREQUAL "FUGAKU")
    target_link_libraries(example3 -lpapi_ext -lpapi -lpfm -Nnofjprof)
  elseif(TARGET_ARCH STREQUAL "FX100")
    target_link_libraries(example3 -lpapi_ext -Wl,'-lpapi,-lpfm')
  else()
    target_link_libraries(example3 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
  endif()
endif()

if(OPT_POWER)
  if(TARGET_ARCH STREQUAL "FUGAKU")
    target_link_libraries(example3 -lpower_ext -lpwr )
  endif()
endif()

if(OPT_OTF)
  target_link_libraries(example3 -lotf_ext -lopen-trace-format)
endif()

set_target_properties(example3 PROPERTIES LINKER_LANGUAGE CXX)

if(with_MPI)
  set (test_parameters -np 2 "example3")
  add_test(NAME TEST_3 COMMAND "mpirun" ${test_parameters})
else()
  add_test(TEST_3 example3)
endif()


### Test 4 : Process group

if(with_MPI)
  add_executable(example4
                 ./test4/main_group_comm.cpp
                 ./test4/sub_copy.c
                 ./test4/sub_kernel.c
  )
  target_link_libraries(example4 -lPMmpi)

  if(OPT_PAPI)
    if(TARGET_ARCH STREQUAL "FUGAKU")
      target_link_libraries(example4 -lpapi_ext -lpapi -lpfm -Nnofjprof)
    elseif(TARGET_ARCH STREQUAL "FX100")
      target_link_libraries(example4 -lpapi_ext -Wl,'-lpapi,-lpfm')
    else()
      target_link_libraries(example4 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
    endif()
  endif()

  if(OPT_POWER)
    if(TARGET_ARCH STREQUAL "FUGAKU")
      target_link_libraries(example4 -lpower_ext -lpwr )
    endif()
  endif()

  if(OPT_OTF)
    target_link_libraries(example4 -lotf_ext -lopen-trace-format)
  endif()

set_target_properties(example4 PROPERTIES LINKER_LANGUAGE CXX)
# target_link_libraries(example4 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})

  set (test_parameters -np 2 "example4")
  add_test(NAME TEST_4 COMMAND "mpirun" ${test_parameters})
endif()


### Test 5 : MPI_Comm_split

if(with_MPI)
  add_executable(example5
                 ./test5/main_split_comm.cpp
                 ./test5/sub_copy.c
                 ./test5/sub_kernel.c
  )
  target_link_libraries(example5 -lPMmpi)

  if(OPT_PAPI)
    if(TARGET_ARCH STREQUAL "FUGAKU")
      target_link_libraries(example5 -lpapi_ext -lpapi -lpfm -Nnofjprof)
    elseif(TARGET_ARCH STREQUAL "FX100")
      target_link_libraries(example5 -lpapi_ext -Wl,'-lpapi,-lpfm')
    else()
      target_link_libraries(example5 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
    endif()
  endif()

  if(OPT_POWER)
    if(TARGET_ARCH STREQUAL "FUGAKU")
      target_link_libraries(example5 -lpower_ext -lpwr )
    endif()
  endif()

  if(OPT_OTF)
    target_link_libraries(example5 -lotf_ext -lopen-trace-format)
  endif()

  set_target_properties(example5 PROPERTIES LINKER_LANGUAGE CXX)
# target_link_libraries(example5 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})

  set (test_parameters -np 2 "example5")
  add_test(NAME TEST_5 COMMAND "mpirun" ${test_parameters})
endif()
