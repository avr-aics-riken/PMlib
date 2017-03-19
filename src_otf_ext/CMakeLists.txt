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

include_directories(${OTF_DIR}/include/open-trace-format)
link_directories(${OTF_DIR}/lib)

C99()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTATIC_PAPI_EVENTS_TABLE -D_REENTRANT -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_COMPILER_TLS  -DPAPI_NO_MEMORY_MANAGEMENT -DUSE_PAPI")

add_library(otf_ext STATIC otf_ext.c)

set_target_properties(otf_ext PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(otf_ext ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES})

install(TARGETS otf_ext DESTINATION lib)
