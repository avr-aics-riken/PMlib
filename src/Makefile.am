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

lib_LIBRARIES = libPM.a

libPM_a_CXXFLAGS = -I$(top_builddir)/include @MPI_CFLAGS@ 

libPM_a_SOURCES = \
    PerfMonitor.cpp \
    PerfWatch.cpp

PM_includedir = $(includedir)
PM_include_HEADERS = \
    $(top_builddir)/include/PerfMonitor.h \
    $(top_builddir)/include/PerfWatch.h \
    $(top_builddir)/include/pmVersion.h


EXTRA_DIST = Makefile_hand depend.inc