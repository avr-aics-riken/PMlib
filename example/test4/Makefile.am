##############################################################################
#
# PMlib - Performance Monitor library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2015 Advanced Institute for Computational Science, RIKEN.
# All rights reserved.
#
##############################################################################

noinst_PROGRAMS = test4


DISTCLEANFILES=*~ test4
CLEANFILES=data/*.log

EXTRA_DIST=

test4_SOURCES  = f_main.f90
test4_FCFLAGS = -I$(top_srcdir)/include -I@PAPI_DIR@/include @MPI_CFLAGS@
test4_CXXFLAGS = -I$(top_srcdir)/include -I@PAPI_DIR@/include @MPI_CFLAGS@
test4_LINK = $(PM_FC_LD) $(CXXFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
#     -std=c99 -g -DSTATIC_PAPI_EVENTS_TABLE -D_REENTRANT -D_GNU_SOURCE -DUSE_COMPILER_TLS  -DPAPI_NO_MEMORY_MANAGEMENT -DUSE_PAPI

test4_LDADD = \
    -L$(top_builddir)/src -lPM@MPI_EXT@ \
    @PM_LIBS@ @MPI_FCLIBS@ \
    @MPI_LIBS@ @MPI_LDFLAGS@

if PAPI_TARGET
  test4_LDADD += \
	  -L$(top_builddir)/src_papi_ext -lpapi_ext \
	  -L@PAPI_DIR@/lib64 -lpapi -lpfm
endif

if OTF_TARGET
  test4_LDADD += \
	-L$(top_builddir)/src_otf_ext -lotf_ext \
	-L@OTF_DIR@/lib -lopen-trace-format
endif

dist_noinst_DATA=

