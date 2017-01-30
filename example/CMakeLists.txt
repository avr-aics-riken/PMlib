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

message(" ")
message("Examples : ")
#message(STATUS "PROJECT_BINARY_DIR = " ${PROJECT_BINARY_DIR})
#message(STATUS "PROJECT_SOURCE_DIR = " ${PROJECT_SOURCE_DIR})
message(STATUS "PAPI_DIR           = " ${PAPI_DIR})
message(STATUS "OTF_DIR            = " ${OTF_DIR})
message(STATUS "with_MPI           = " ${with_MPI})
message(STATUS "IMPLICIT_LINK_LIBS = " ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})


include_directories(${PROJECT_BINARY_DIR}/include ${PROJECT_SOURCE_DIR}/include)

link_directories(${PROJECT_BINARY_DIR}/src)

if(with_PAPI)
  link_directories(${PROJECT_BINARY_DIR}/src_otf_ext)
endif()

if(with_OTF)
  link_directories(${PROJECT_BINARY_DIR}/src_papi_ext)
endif()


### Test 3 and 5 are only for MPI environment. Others are for both serial and MPI.

#### Test1 : C++

add_executable(example1 ./test1/main_pmlib.cpp)

if(with_MPI)
  target_link_libraries(example1 -lPMmpi)
else()
  target_link_libraries(example1 -lPM)
endif()

if(with_PAPI)
  link_directories(${PAPI_DIR}/lib64)
  target_link_libraries(example1 -lpapi_ext -lpapi -lpfm)
endif()

if(with_OTF)
  link_directories(${OTF_DIR}/lib)
  target_link_libraries(example1 -lotf_ext -lopen-trace-format)
endif()

add_test(TEST_1 example1)


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

if(with_PAPI)
  link_directories(${PAPI_DIR}/lib64)
  target_link_libraries(example2 -lpapi_ext -lpapi -lpfm)
endif()

if(with_OTF)
  link_directories(${OTF_DIR}/lib)
  target_link_libraries(example2 -lotf_ext -lopen-trace-format)
endif()

set_target_properties(example2 PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(example2 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
add_test(TEST_2 example2)


### Test 3 : Process group

if(with_MPI)
  add_executable(example3
                 ./test3/main_group_comm.cpp
                 ./test3/sub_copy.c
                 ./test3/sub_kernel.c
  )
  target_link_libraries(example3 -lPMmpi)

  if(with_PAPI)
    link_directories(${PAPI_DIR}/lib64)
    target_link_libraries(example3 -lpapi_ext -lpapi -lpfm)
  endif()

  if(with_OTF)
    link_directories(${OTF_DIR}/lib)
    target_link_libraries(example3 -lotf_ext -lopen-trace-format)
  endif()

  set_target_properties(example3 PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(example3 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
  add_test(TEST_3 example3)
endif()


### Test 4 : Fortran

if(enable_Fortran)
  add_executable(example4 ./test4/f_main.f90)
  # CXXでリンク
  set_target_properties(example4 PROPERTIES LINKER_LANGUAGE CXX)

  if(with_MPI)
    target_link_libraries(example4 -lPMmpi)
  else()
    target_link_libraries(example4 -lPM)
  endif()

  if(with_PAPI)
    link_directories(${PAPI_DIR}/lib64)
    target_link_libraries(example4 -lpapi_ext -lpapi -lpfm)
  endif()

  if(with_OTF)
    link_directories(${OTF_DIR}/lib)
    target_link_libraries(example4 -lotf_ext -lopen-trace-format)
  endif()

  # Fujitsuの場合　--linkfortranは必須
  if(CMAKE_CXX_COMPILER MATCHES ".*FCCpx$")
    target_link_libraries(example4 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES} "--linkfortran")
  else()
    target_link_libraries(example4 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
  endif()

  add_test(TEST_4 example4)

endif()


### Test 5 : MPI_Comm_split

if(with_MPI)
  add_executable(example5
                 ./test5/main_split_comm.cpp
                 ./test5/sub_copy.c
                 ./test5/sub_kernel.c
  )
  target_link_libraries(example5 -lPMmpi)

  if(with_PAPI)
    link_directories(${PAPI_DIR}/lib64)
    target_link_libraries(example5 -lpapi_ext -lpapi -lpfm)
  endif()

  if(with_OTF)
    link_directories(${OTF_DIR}/lib)
    target_link_libraries(example5 -lotf_ext -lopen-trace-format)
  endif()

  set_target_properties(example5 PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(example5 ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})
  add_test(TEST_5 example5)
endif()