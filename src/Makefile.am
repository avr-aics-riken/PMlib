###
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
###

lib_LIBRARIES = libPM@MPI_EXT@.a

libPM_a_CXXFLAGS = @MPI_CFLAGS@ -I$(top_builddir)/include -I@PAPI_DIR@/include

AM_CXXFLAGS = $(libPM_a_CXXFLAGS)

libPM@MPI_EXT@_a_SOURCES = \
    PerfMonitor.cpp \
    PerfWatch.cpp \
    PerfCpuType.cpp

nobase_include_HEADERS = \
    $(top_builddir)/include/PerfMonitor.h \
    $(top_builddir)/include/PerfWatch.h \
    $(top_builddir)/include/pmVersion.h \
    $(top_builddir)/include/pmlib_papi.h

EXTRA_DIST = Makefile_hand depend.inc
