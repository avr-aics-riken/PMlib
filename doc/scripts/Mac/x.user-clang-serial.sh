#!/bin/bash
set -x

PMLIB_DIR=/usr/local/pmlib
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

OTF_DIR=/usr/local/otf/otf-1.12.5-clang
OTF_LDFLAGS="-lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

INCLUDES="${PMLIB_INCLUDES} "
LDFLAGS="${PMLIB_LDFLAGS} ${OTF_LDFLAGS} "

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/*

PACKAGE_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/package
cp ${PACKAGE_DIR}/doc/src_tutorial/main_mpi.cpp  main.cpp

CXXFLAGS="-DDISABLE_MPI=true"
c++ ${CXXFLAGS} ${INCLUDES} -o main.ex main.cpp ${LDFLAGS}

./main.ex

