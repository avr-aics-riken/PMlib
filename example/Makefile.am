##############################################################################
#
# PMlib - Performance Monitor library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2014 Advanced Institute for Computational Science, RIKEN.
# All rights reserved.
#
##############################################################################

noinst_PROGRAMS = check_new_api pmlib_test


DISTCLEANFILES=*~ check_new_api pmlib_test
CLEANFILES=data/*.log

EXTRA_DIST= check_new_api.c sub_kernel.c pmlib_test.cpp


check_new_api_SOURCES  =  check_new_api.c
check_new_api_CFLAGS = -I$(top_builddir)/include -I@PAPI_DIR@/include
#     -std=c99 -g -DSTATIC_PAPI_EVENTS_TABLE -D_REENTRANT -D_GNU_SOURCE -DUSE_COMPILER_TLS  -DPAPI_NO_MEMORY_MANAGEMENT -DUSE_PAPI

check_new_api_LDADD = \
    -L$(top_builddir)/src -lPM \
    -L$(top_builddir)/src_papi_ext -lpapi_ext \
    @MPI_LIBS@ \
    @MPI_LDFLAGS@ \
    -L@PAPI_DIR@/lib64 -lpapi \
    -L@PAPI_DIR@/lib64 -lpfm

pmlib_test_SOURCES  = pmlib_test.cpp sub_kernel.c
pmlib_test_CFLAGS = -I$(top_builddir)/include -I@PAPI_DIR@/include @MPI_CFLAGS@
pmlib_test_CXXFLAGS = -I$(top_builddir)/include -I@PAPI_DIR@/include @MPI_CFLAGS@
#     -std=c99 -g -DSTATIC_PAPI_EVENTS_TABLE -D_REENTRANT -D_GNU_SOURCE -DUSE_COMPILER_TLS  -DPAPI_NO_MEMORY_MANAGEMENT -DUSE_PAPI

pmlib_test_LDADD = \
    -L$(top_builddir)/src -lPM \
    -L$(top_builddir)/src_papi_ext -lpapi_ext \
    @MPI_LIBS@ \
    @MPI_LDFLAGS@ \
    -L@PAPI_DIR@/lib64 -lpapi \
    -L@PAPI_DIR@/lib64 -lpfm

dist_noinst_DATA=

.PHONY:$(dist_noinst_DATA)



