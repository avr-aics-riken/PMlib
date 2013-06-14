###################################################################
#
# PMlib - Performance Monitor library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2013 Advanced Institute for Computational Science, RIKEN.
# All rights reserved.
#
###################################################################


##########
# FX10, K
##########

AR          = ar cr
RANLIB      = ranlib
RM          = \rm -f
MPI_DIR     =
CXX         = mpiFCCpx
CXXFLAGS    = -Kfast
CXXFLAGS   += -I$(MPI_DIR)/include

