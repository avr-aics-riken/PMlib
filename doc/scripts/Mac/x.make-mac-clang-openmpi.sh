#!/bin/bash
set -x
date
hostname
#	export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.8.7-clang
export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.10.2-clang
export PATH=${PATH}:${OPENMPI_DIR}/bin
#	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${OPENMPI_DIR}/lib
export MPI_DIR=${OPENMPI_DIR}

INSTALL_DIR=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_mac
SRC_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/PMlib

BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean >/dev/null 2>&1

FCFLAGS="-cpp"
CFLAGS=""
CXXFLAGS=""

../configure \
	CXX=mpicxx CC=mpicc FC=mpifort \
	CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" FCFLAGS="${FCFLAGS}" \
	--prefix=${INSTALL_DIR} \
	--with-ompi=${MPI_DIR} \
	--with-papi=none \
	--with-example=yes
	#  >/dev/null 2>&1
	#	CXX=mpic++ CC=mpicc FC=mpifort \


make
sleep 3
mpicxx --version
mpicc --version
mpifort --version
which mpifort 
make install # may be done by root
exit

NPROCS=2
#N.A. export OMP_NUM_THREADS=2
#N.A. export HWPC_CHOOSER=FLOPS
time mpirun -np ${NPROCS} example/test1/test1
sleep 3
time mpirun -np ${NPROCS} example/test2/test2
sleep 3
time mpirun -np ${NPROCS} example/test3/test3
sleep 3
time mpirun -np ${NPROCS} example/test4/test4
sleep 3
time mpirun -np ${NPROCS} example/test5/test5
sleep 3
exit

make install # may be done by root
if [ $? != 0 ] ; then echo '@@@ installation error @@@'; exit; fi

