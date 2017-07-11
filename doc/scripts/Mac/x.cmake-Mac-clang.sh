#!/bin/bash
set -x
date
hostname
export OPENMPI_DIR=/usr/local/openmpi/openmpi-1.10.2-clang
export PATH=${PATH}:${OPENMPI_DIR}/bin

PACKAGE_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/package
PMLIB_DIR=/usr/local/pmlib

OTF_DIR="no"
#	OTF_DIR=/usr/local/otf/otf-1.12.5-clang
#	export LD_LIBRARY_PATH+=":${OTF_DIR}/lib"

BUILD_DIR=${PACKAGE_DIR}/BUILD_DIR
mkdir -p $BUILD_DIR
cd ${BUILD_DIR}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi


# 1. Serial version

export CC=cc CXX=c++ F90=gfortran FC=gfortran
FCFLAGS="-cpp"
CFLAGS=""
CXXFLAGS=""
rm -rf  ${BUILD_DIR}/*

cmake \
	-DINSTALL_DIR=${INSTALL_DIR} \
	-DCMAKE_Fortran_FLAGS="${FCFLAGS}" \
	-Denable_OPENMP=no \
	-Dwith_MPI=no \
	-Denable_Fortran=yes \
	-Dwith_example=yes \
	-Dwith_PAPI=no \
	-Dwith_OTF=${OTF_DIR} \
	..


make VERBOSE=1
cmake -version
make install


# 2. MPI version

export CC=mpicc CXX=mpicxx F90=mpif90 FC=mpif90

FCFLAGS="-cpp"
CFLAGS=""
CXXFLAGS=""
rm -rf  ${BUILD_DIR}/*

cmake \
	-DINSTALL_DIR=${PMLIB_DIR} \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
	-DCMAKE_Fortran_FLAGS="${FCFLAGS}" \
	-Denable_OPENMP=no \
	-Dwith_MPI=yes \
	-Denable_Fortran=yes \
	-Dwith_example=yes \
	-Dwith_PAPI=no \
	-Dwith_OTF=${OTF_DIR} \
	..

make VERBOSE=1
make install
if [ $? != 0 ] ; then echo '@@@ installation error @@@'; exit; fi

RUN_EXAMPLE="yes"
if [ ${RUN_EXAMPLE} == yes ] ; then
	NPROCS=2
	export OTF_TRACING=full	# full|on|off
	export OTF_FILENAME="check_OTF"
	mpiexec -n ${NPROCS} example/example1
	pwd
	ls -go
	cat *.def
fi
