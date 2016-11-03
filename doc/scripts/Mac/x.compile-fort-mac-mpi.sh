#!/bin/bash
set -x
export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.10.2-clang
export PATH=${PATH}:${OPENMPI_DIR}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${OPENMPI_DIR}/lib

PMLIB_ROOT=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_mac
PMLIB_INCLUDES="-I${PMLIB_ROOT}/include "
INCLUDES="${PMLIB_INCLUDES} "
PMLIB_LDFLAGS="-L${PMLIB_ROOT}/lib -lPMmpi "
MPI_CXX_LIB="-lmpi_cxx "	# Needed if linking fortran MPI driver with C++ libs
LDFLAGS="${PMLIB_LDFLAGS} ${MPI_CXX_LIB}"

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

MY_SRC=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/scripts.mac/src_tests
cp ${MY_SRC}/f_main.f90 main.f90

FCFLAGS="-cpp -fopenmp"
mpifort ${FCFLAGS} ${INCLUDES} -o main.ex main.f90 ${LDFLAGS}

NPROCS=2
export OMP_NUM_THREADS=2
mpirun -np ${NPROCS} main.ex

#	CXXFLAGS="-fopenmp"
#	mpicxx ${CXXFLAGS} ${INCLUDES} -o main.ex main.cpp ${LDFLAGS}
