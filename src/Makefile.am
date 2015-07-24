###
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
###

lib_LIBRARIES = libPM@MPI_EXT@.a

libPM@MPI_EXT@_a_CXXFLAGS = @MPI_CFLAGS@ -I$(top_srcdir)/include -I@PAPI_DIR@/include

AM_CXXFLAGS = $(libPM@MPI_EXT@_a_CXXFLAGS)

libPM@MPI_EXT@_a_SOURCES = \
    PerfMonitor.cpp \
    PerfWatch.cpp \
    PerfCpuType.cpp \
    PerfProgFortran.cpp

include_HEADERS = \
    $(top_srcdir)/include/PerfMonitor.h \
    $(top_srcdir)/include/PerfWatch.h \
    $(top_srcdir)/include/pmVersion.h \
    $(top_srcdir)/include/pmlib_papi.h

EXTRA_DIST = Makefile_hand depend.inc
