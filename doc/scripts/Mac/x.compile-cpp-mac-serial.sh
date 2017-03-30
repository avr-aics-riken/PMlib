#!/bin/bash
set -x
PMLIB_ROOT=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_mac

PMLIB_INCLUDES="-I${PMLIB_ROOT}/include "
INCLUDES="${PMLIB_INCLUDES} "
PMLIB_LDFLAGS="-L${PMLIB_ROOT}/lib -lPM "
LDFLAGS="${PMLIB_LDFLAGS} ${MPI_CXX_LIB}"

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

MY_SRC=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/PMlib/doc/src_tutorial
cp ${MY_SRC}/mxm.cpp main.cpp


CFLAGS="-DDISABLE_MPI"
c++    ${CFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}
./a.out
