#!/bin/bash
export PATH=/opt/local/bin:/opt/local/sbin:${PATH}
#	export PATH=${PATH}:/opt/local/bin:/opt/local/sbin

export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.8.7-gcc
export PATH=${PATH}:${OPENMPI_DIR}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${OPENMPI_DIR}/lib
export MPI_DIR=${OPENMPI_DIR}

set -x
INSTALL_DIR=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_gnu
SRC_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/PMlib
#	SRC_DIR=${HOME}/Desktop/Git_Repos/pmlib/avr-aics-riken/PMlib
BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean >/dev/null 2>&1

FCFLAGS="-cpp"
CFLAGS=""
CXXFLAGS=""

#	FCFLAGS="${FCFLAGS} -DDEBUG_FORT"
#	CFLAGS="${CFLAGS} -DDEBUG_PRINT_PAPI -DDEBUG_PRINT_LABEL"
#	CXXFLAGS="${CXXFLAGS} -DDEBUG_PRINT_PAPI -DDEBUG_PRINT_LABEL -DDEBUG_PRINT_PAPI_THREADS -DDEBUG_PRINT_VERSION"

	#CXX=g++ CC=gcc FC=gfortran \

../configure \
	CXX=mpicxx CC=mpicc FC=mpifort \
	CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" FCFLAGS="${FCFLAGS}" \
	--prefix=${INSTALL_DIR} \
	--with-comp=GNU \
	--with-ompi=${MPI_DIR} \
	--with-papi=none \
	--with-example=yes
	#  >/dev/null 2>&1

make clean >/dev/null 2>&1
make
sleep 3

NPROCS=2
export OMP_NUM_THREADS=2
export HWPC_CHOOSER=FLOPS
#	mpirun -np ${NPROCS} example/test1/test1
#	sleep 5
#	mpirun -np ${NPROCS} example/test2/test2
#	sleep 5
#	mpirun -np ${NPROCS} example/test3/test3
#	sleep 3
mpirun -np ${NPROCS} example/test4/test4
#	sleep 3
#	mpirun -np ${NPROCS} example/test5/test5
#	sleep 3
exit

make install # may be done by root
if [ $? != 0 ] ; then echo '@@@ installation error @@@'; exit; fi

