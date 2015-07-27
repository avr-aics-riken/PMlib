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


all: pmlib_test.ex

pmlib_test.ex: pmlib_test.o 
	$(CXX) $(CXXFLAGS) $(PM_INCLUDES) -o $@ pmlib_test.o $(PMLIB_LDFLAGS)

CXX = mpiFCCpx
CXXFLAGS = -Kopenmp,fast -DUSE_PAPI
CC = mpifccpx
CFLAGS = -Kopenmp,fast -DUSE_PAPI
PM_INCLUDES = -I../include -I/opt/FJSVXosDevkit/sparc64fx/V01L02E07/target/usr/include
PM_DIR = ../.
PMLIB_LDFLAGS = \
    -L$(PM_DIR)/src -lPM \
    -L$(PM_DIR)/src_papi_ext -lpapi_ext \
    -L/opt/FJSVXosDevkit/sparc64fx/V01L02E07/target/usr/lib64 -lpapi -lpfm

.c.o:
	$(CC) $(CFLAGS) $(PM_INCLUDES) -c $<

.cpp.o:
	$(CXX) $(CXXFLAGS) $(PM_INCLUDES) -c $<

clean:
	$(RM) *.o 

