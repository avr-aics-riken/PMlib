#!/bin/bash
set -x
export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.10.2-clang
export PATH=${PATH}:${OPENMPI_DIR}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${OPENMPI_DIR}/lib
#	export CC=gcc CXX=g++ FC=gfortran

mpicxx --version

PMLIB_DIR=/usr/local/pmlib
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi "

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

mpicxx ${CXXFLAGS} ${INCLUDES} -o main.ex main.cpp ${LDFLAGS}

NPROCS=2
export OMP_NUM_THREADS=2
mpirun -np ${NPROCS} main.ex

