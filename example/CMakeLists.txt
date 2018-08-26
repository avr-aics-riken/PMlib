##################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2018 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2018 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################

message(" ")
message("Examples : ")
message(STATUS "PAPI_DIR            = " ${PAPI_DIR})
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

if(OPT_OTF)
  include_directories(${OTF_DIR}/include/open-trace-format)
  link_directories(${PROJECT_BINARY_DIR}/src_otf_ext)
  link_directories(${OTF_DIR}/lib)
endif()


### Test 3 and 5 are only for MPI environment. Others are for both serial and MPI.

#### Test1 : C++

add_executable(example1 ./test1/main_pmlib.cpp)

if(with_MPI)
  target_link_libraries(example1 -lPMmpi)
else()
  target_link_libraries(example1 -lPM)
endif()

if(OPT_PAPI)
  if(TARGET_ARCH STREQUAL "FX100")
  # on AICS FX100 static PAPI libs does not match the other system libs.
  # so the following dynamic libs must be used. bonehead Fujitsu...
    target_link_libraries(example1 -lpapi_ext -Wl,'-lpapi,-lpfm')
  else()
  ## GNU linker ld options are better passed using -Wl, rather than -B compiler phase option
    target_link_libraries(example1 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
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



### Test2 : C++ and C

add_executable(example2
               ./test2/main_kernel.cpp
               ./test2/sub_kernel.c
)

if(with_MPI)
  target_link_libraries(example2 -lPMmpi)
else()
  target_link_libraries(example2 -lPM)
endif()

if(OPT_PAPI)
  if(TARGET_ARCH STREQUAL "FX100")
    target_link_libraries(example2 -lpapi_ext -Wl,'-lpapi,-lpfm')
  else()
    target_link_libraries(example2 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
  endif()
endif()

if(OPT_OTF)
  target_link_libraries(example2 -lotf_ext -lopen-trace-format)
endif()

set_target_properties(example2 PROPERTIES LINKER_LANGUAGE CXX)
#	On vsh, CMAKE_CXX_IMPLICIT_LINK_LIBRARIES contains -lmpi -lmpigf -lmpigi, etc
#	They are not threaded and result in link errors on vsh if OpenMP is enabled.
#	Temporary sotution is to exclude them.
# target_link_libraries(example2 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})

if(with_MPI)
  set (test_parameters -np 2 "example2")
  add_test(NAME TEST_2 COMMAND "mpirun" ${test_parameters})
else()
  add_test(TEST_2 example2)
endif()


### Test 3 : Process group

if(with_MPI)
  add_executable(example3
                 ./test3/main_group_comm.cpp
                 ./test3/sub_copy.c
                 ./test3/sub_kernel.c
  )
  target_link_libraries(example3 -lPMmpi)

  if(OPT_PAPI)
    if(TARGET_ARCH STREQUAL "FX100")
      target_link_libraries(example3 -lpapi_ext -Wl,'-lpapi,-lpfm')
    else()
      target_link_libraries(example3 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
    endif()
  endif()

  if(OPT_OTF)
    target_link_libraries(example3 -lotf_ext -lopen-trace-format)
  endif()

set_target_properties(example3 PROPERTIES LINKER_LANGUAGE CXX)
# target_link_libraries(example3 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})

  set (test_parameters -np 2 "example3")
  add_test(NAME TEST_3 COMMAND "mpirun" ${test_parameters})
endif()


### Test 4 : Fortran

if(enable_Fortran)
  add_executable(example4 ./test4/f_main.f90)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  elseif(CMAKE_Fortran_COMPILER_ID STREQUAL "PGI")
    set_target_properties(example4 PROPERTIES LINKER_LANGUAGE Fortran)
    target_link_libraries(example4 -pgf90libs)
  else()
    set_target_properties(example4 PROPERTIES LINKER_LANGUAGE CXX)
  endif()

  if(with_MPI)
    target_link_libraries(example4 -lPMmpi)
  else()
    target_link_libraries(example4 -lPM)
  endif()

  if(OPT_PAPI)
    if(TARGET_ARCH STREQUAL "FX100")
      target_link_libraries(example4 -lpapi_ext -Wl,'-lpapi,-lpfm')
    else()
      target_link_libraries(example4 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
    endif()
  endif()

  if(OPT_OTF)
    target_link_libraries(example4 -lotf_ext -lopen-trace-format)
  endif()

  # Fujitsuの場合　--linkfortranは必須
  if(USE_F_TCS STREQUAL "YES")
    target_link_libraries(example4 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES} "--linkfortran")
  else()
#   target_link_libraries(example4 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
    target_link_libraries(example4 -lstdc++ )
  endif()

  if(with_MPI)
    set (test_parameters -np 2 "example4")
    add_test(NAME TEST_4 COMMAND "mpirun" ${test_parameters})
  else()
    add_test(TEST_4 example4)
  endif()

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
    if(TARGET_ARCH STREQUAL "FX100")
      target_link_libraries(example5 -lpapi_ext -Wl,'-lpapi,-lpfm')
    else()
      target_link_libraries(example5 -lpapi_ext -Wl,'-Bstatic,-lpapi,-lpfm,-Bdynamic')
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
