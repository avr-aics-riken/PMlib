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
#
#  At first, editing a make_setting.* file, then make

# Install Dir
PM_DIR = /usr/local/PMlib

# Default environment
MACHINE=intel


### compiler detection

ifeq ($(MACHINE),gnu)
include make_setting.gnu
endif

ifeq ($(MACHINE),fx)
include make_setting.fx
endif

ifeq ($(MACHINE),ibm)
include make_setting.ibm
endif

ifeq ($(MACHINE),intel)
include make_setting.intel
endif



all:
	( \
	cd src; \
	make \
		CXX='$(CXX)' \
		CXXFLAGS='$(CXXFLAGS)' \
		AR='$(AR)' \
		RANLIB='$(RANLIB)' \
		RM='$(RM)' \
		PM_DIR='$(PM_DIR)' \
	)

clean:
	(cd src; make clean)

depend:
	(cd src; make depend)
