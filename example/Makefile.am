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

noinst_PROGRAMS = pmlib_test


DISTCLEANFILES=*~ pmlib_test
CLEANFILES=data/*.log

EXTRA_DIST= pmlib_test.cpp Makefile_hand.fx10.login Makefile_hand.intel


pmlib_test_SOURCES  = pmlib_test.cpp 
pmlib_test_CFLAGS = -I$(top_builddir)/include -I@PAPI_DIR@/include @MPI_CFLAGS@
pmlib_test_CXXFLAGS = -I$(top_builddir)/include -I@PAPI_DIR@/include @MPI_CFLAGS@
#     -std=c99 -g -DSTATIC_PAPI_EVENTS_TABLE -D_REENTRANT -D_GNU_SOURCE -DUSE_COMPILER_TLS  -DPAPI_NO_MEMORY_MANAGEMENT -DUSE_PAPI

pmlib_test_LDADD = \
    -L$(top_builddir)/src -lPM@MPI_EXT@ \
    @MPI_LIBS@ \
    @MPI_LDFLAGS@ \
    @PM_LIBS@

dist_noinst_DATA=

.PHONY:$(dist_noinst_DATA)



