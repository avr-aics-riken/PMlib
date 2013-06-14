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


PMLIB  = libPM.a

all: $(PMLIB)

OBJS = PerfWatch.o PerfMonitor.o


$(PMLIB): $(OBJS)
	$(AR) $(PMLIB) $(OBJS) 
	$(RANLIB) $(PMLIB)
	-mkdir -p $(PM_DIR)/lib
	mv $(PMLIB) $(PM_DIR)/lib
	-mkdir -p $(PM_DIR)/include
	cp ../include/*.h $(PM_DIR)/include
	-mkdir -p $(PM_DIR)/doc
	cp ../doc/*.pdf $(PM_DIR)/doc

.cpp.o:
	$(CXX) $(CXXFLAGS) -I../include -c $<

clean:
	$(RM) *.o $(PM_DIR)/$(PMLIB)

depend:
	makedepend -I. -I../include -Y PerfWatch.cpp PerfMonitor.cpp
  
# DO NOT DELETE

PerfWatch.o: ../include/PerfWatch.h
PerfMonitor.o: ../include/PerfMonitor.h ../include/PerfWatch.h
