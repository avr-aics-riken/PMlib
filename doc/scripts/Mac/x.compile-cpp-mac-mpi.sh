#!/bin/bash
set -x
#	export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.10.2-gnu
export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.10.2-clang
export PATH=${PATH}:${OPENMPI_DIR}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${OPENMPI_DIR}/lib
export  FC=gfortran
export  CC=gcc
export  CXX=g++

PMLIB_ROOT=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_mac

PMLIB_INCLUDES="-I${PMLIB_ROOT}/include "
INCLUDES="${PMLIB_INCLUDES} "
PMLIB_LDFLAGS="-L${PMLIB_ROOT}/lib -lPMmpi "
MPI_CXX_LIB="-lmpi_cxx "	# Needed if linking fortran MPI driver with C++ libs
LDFLAGS="${PMLIB_LDFLAGS} ${MPI_CXX_LIB}"

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

MY_SRC=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/PMlib/doc/src_tutorial
cp ${MY_SRC}/mpi_pmlib.cpp main.cpp


CFLAGS=""
mpicxx    ${CFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}

NPROCS=2
export OMP_NUM_THREADS=2
mpirun -np ${NPROCS} ./a.out

#	CXXFLAGS="-fopenmp"
#	mpicxx ${CXXFLAGS} ${INCLUDES} -o main.ex main.cpp ${LDFLAGS}
