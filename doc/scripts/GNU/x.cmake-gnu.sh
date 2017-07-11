#!/bin/bash
set -x
date
hostname

#	OPENMPI_DIR=${HOME}/openmpi/openmpi-1.10.3
OPENMPI_DIR=${HOME}/openmpi/usr_local_openmpi
MPI_DIR=${OPENMPI_DIR}
export  PATH=${OPENMPI_DIR}/bin:${PATH}
export  LD_LIBRARY_PATH=${OPENMPI_DIR}/lib:${LD_LIBRARY_PATH}

PACKAGE_DIR=${HOME}/pmlib/package
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/gnu

#	PAPI_DIR="no"
PAPI_DIR=${HOME}/papi/usr_local_papi/papi-5.5.1-gnu
#Remark-- PAPI 5.5 contains ffsll() which is not compatible with Linux.
#Remark-- Comment out ${PAPI_DIR}/include/papi.h line 1024, i.e. ffsll()

OTF_DIR="no"
#	OTF_DIR=${HOME}/otf/usr_local_otf/otf-1.12-gnu

BUILD_DIR=${PACKAGE_DIR}/BUILD_DIR
mkdir -p ${BUILD_DIR}
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm -rf  ${BUILD_DIR}/*

CXXFLAGS="-fopenmp -w "
CFLAGS="-std=c99 -fopenmp -w "
FFLAGS="-cpp -fopenmp -w "

# 1. MPI version

export CC=mpicc CXX=mpicxx F90=mpif90 FC=mpif90

cmake \
	-DINSTALL_DIR=${PMLIB_DIR} \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
	-DCMAKE_C_FLAGS="${CFLAGS}" \
	-DCMAKE_Fortran_FLAGS="${FFLAGS}" \
	-Dwith_MPI=yes \
	-Denable_Fortran=yes \
	-Denable_OPENMP=yes \
	-Dwith_example=yes \
	-Dwith_PAPI="${PAPI_DIR}" \
	-Dwith_OTF="${OTF_DIR}" \
	..

make VERBOSE=1
make install

RUN_EXAMPLE="no"
if [ ${RUN_EXAMPLE} == yes ] ; then
	NPROCS=2
	export OMP_NUM_THREADS=4
	export HWPC_CHOOSER=FLOPS
	export OTF_TRACING=full
	export OTF_FILENAME="trace_pmlib"
	mpirun -np ${NPROCS} example/example1
	ls -l
fi

# 2. Serial version

export CC=gcc CXX=g++ F90=gfortran FC=gfortran

cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm -rf  ${BUILD_DIR}/*

cmake \
	-DINSTALL_DIR=${PMLIB_DIR} \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
	-DCMAKE_C_FLAGS="${CFLAGS}" \
	-DCMAKE_Fortran_FLAGS="${FFLAGS}" \
	-Dwith_MPI=no \
	-Denable_Fortran=yes \
	-Denable_OPENMP=yes \
	-Dwith_example=yes \
	-Dwith_PAPI="${PAPI_DIR}" \
	-Dwith_OTF="${OTF_DIR}" \
	..


make VERBOSE=1
make install

RUN_EXAMPLE="no"
if [ ${RUN_EXAMPLE} == yes ] ; then
	export OMP_NUM_THREADS=4
	export HWPC_CHOOSER=FLOPS
	export OTF_TRACING=full
	export OTF_FILENAME="trace_pmlib"
	example/example1
ls -l
fi

