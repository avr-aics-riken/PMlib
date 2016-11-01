#!/bin/bash
set -x

PMLIB_ROOT=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_mac
PMLIB_INCLUDES="-I${PMLIB_ROOT}/include "
INCLUDES="${PMLIB_INCLUDES} "
PMLIB_LDFLAGS="-L${PMLIB_ROOT}/lib -lPM "
CXX_LIB="-lc++"
LDFLAGS="${PMLIB_LDFLAGS} ${CXX_LIB}"

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

rm a.out main.ex

#MY_SRC=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/scripts.mac/src_tests
MY_SRC=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/PMlib/doc/src_tutorial
cp ${MY_SRC}/mxm.f90 main.f90

FCFLAGS="-cpp -fopenmp -D_PM_WITHOUT_MPI_"
gfortran ${FCFLAGS} ${INCLUDES} -o main.ex main.f90 ${LDFLAGS}

export OMP_NUM_THREADS=2
./main.ex

