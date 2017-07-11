#!/bin/bash
set -x
export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.10.2-clang
export PATH=${PATH}:${OPENMPI_DIR}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${OPENMPI_DIR}/lib
export CC=gcc CXX=g++ FC=gfortran

PMLIB_DIR=/usr/local/pmlib
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi "

OTF_DIR=/usr/local/otf/otf-1.12.5-clang
OTF_LDFLAGS="-lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

LDFLAGS="${PMLIB_LDFLAGS} ${OTF_LDFLAGS} -lmpi_cxx"
# -lmpi_cxx is needed when linking fortran MPI driver with C++

INCLUDES="${PMLIB_INCLUDES} "

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/*

PACKAGE_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/package
cp ${PACKAGE_DIR}/doc/src_tutorial/f_main.f90 main.f90

FCFLAGS="-cpp -fopenmp"
mpifort ${FCFLAGS} ${INCLUDES} -o main.ex main.f90 ${LDFLAGS}

NPROCS=2
export OMP_NUM_THREADS=2
mpirun -np ${NPROCS} main.ex


