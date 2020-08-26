#!/bin/bash
# eagles
#	module purge
#	module load intelcompiler intelmpi
# chicago
#	INTEL_TOP=/opt/intel
#	source ${INTEL_TOP}/bin/compilervars.sh intel64
# ito
#	module load intel
# general cluster
#	INTEL_DIR=/opt/intel/2018
#	source ${INTEL_DIR}/bin/compilervars.sh intel64
#	I_MPI_ROOT=/opt/intel/impi
#	source ${MPI_DIR}/bin64/mpivars.sh

set -x
umask 0022
export LANG=C
#	export PATH=${HOME}/cmake/cmake-2.8.12.2/build/bin:${PATH}

PACKAGE_DIR=${HOME}/pmlib/package
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-intel

#	PAPI_DIR="no"
PAPI_DIR=${HOME}/papi/usr_local_papi/papi-5.6.0-intel
#Remark-- PAPI 5.5 contains ffsll() which is not compatible with Linux.
#Remark-- Comment out ${PAPI_DIR}/include/papi.h line 1024, i.e. ffsll()

OTF_DIR="no"
#	OTF_DIR=${HOME}/otf/usr_local_otf/otf-1.12.5-intel

BUILD_DIR=${PACKAGE_DIR}/BUILD_DIR
mkdir -p ${BUILD_DIR}
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm -rf  ${BUILD_DIR}/*

CXXFLAGS="-qopenmp "
CFLAGS="-std=c99 -qopenmp "
FCFLAGS="-fpp -qopenmp "

# 1. serial version

export CC=icc CXX=icpc F90=ifort FC=ifort

cmake \
	-DINSTALL_DIR=${PMLIB_DIR} \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
	-DCMAKE_C_FLAGS="${CFLAGS}" \
	-DCMAKE_Fortran_FLAGS="${FCFLAGS}" \
	-Dwith_MPI=no \
	-Denable_Fortran=yes \
	-Denable_OPENMP=yes \
	-Dwith_example=yes \
	-Dwith_PAPI="${PAPI_DIR}" \
	-Dwith_OTF="${OTF_DIR}" \
	..

make VERBOSE=1

RUN_EXAMPLE="no"
	if [ ${RUN_EXAMPLE} == yes ] ; then
	export OMP_NUM_THREADS=4
	export HWPC_CHOOSER=FLOPS
	#	export OTF_TRACING=full
	#	export OTF_FILENAME="trace_pmlib"
	example/example1
	ls -l
	fi

make install

# 2. MPI version

export CC=mpiicc CXX=mpiicpc F90=mpiifort FC=mpiifort
export I_MPI_CC=icc I_MPI_CXX=icpc I_MPI_F90=ifort I_MPI_FC=ifort

cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm -rf  ${BUILD_DIR}/*

cmake \
	-DINSTALL_DIR=${PMLIB_DIR} \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
	-DCMAKE_C_FLAGS="${CFLAGS}" \
	-DCMAKE_Fortran_FLAGS="${FCFLAGS}" \
	-Dwith_MPI=yes \
	-Denable_Fortran=yes \
	-Denable_OPENMP=yes \
	-Dwith_example=yes \
	-Dwith_PAPI="${PAPI_DIR}" \
	-Dwith_OTF="${OTF_DIR}" \
	..

make VERBOSE=1

RUN_EXAMPLE="no"
	if [ ${RUN_EXAMPLE} == yes ] ; then
	NPROCS=4
	export OMP_NUM_THREADS=4
	export HWPC_CHOOSER=FLOPS
	#	export OTF_TRACING=full
	#	export OTF_FILENAME="trace_pmlib"
	mpirun -np ${NPROCS} example/example1
	fi

make install

