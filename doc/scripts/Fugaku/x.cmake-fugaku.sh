#!/bin/bash
#	module load lang
#	module load lang/fjcompiler20200214_01
set -x
export LANG=C

PACKAGE_DIR=${HOME}/pmlib/package
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fugaku

#	PAPI_DIR="no"
#	PAPI_DIR="yes"	# doesn't work on fugaku
PAPI_DIR=/opt/FJSVxos/devkit/aarch64/rfs/usr	# login node
#	PAPI libraries are on
# location on login nodes : /opt/FJSVxos/devkit/aarch64/rfs/usr/lib64/libpapi.so
# location on compute nodes : /usr/lib64/libpapi.so


#	On K/FX100, PAPI library is automatically searched by mpi*px compilers.
#	So, setting "yes" is good enough to link PAPI.

OTF_DIR="no"
#	OTF_DIR=${HOME}/otf/usr_local_otf/aarch64


#	CXXFLAGS="-DUSE_PRECISE_TIMER -Nfjcex "
#	CXXFLAGS="-Kocl "
CXXFLAGS="-Kocl -Nnofjprof -Nfjomplib "
CFLAGS="-Kocl -Nnofjprof -Nfjomplib "
FFLAGS="-Kocl -Nnofjprof -Nfjomplib "
DEBUG=" "

# 1. Serial version

BUILD_DIR=${PACKAGE_DIR}/BUILD_SERIAL
mkdir -p $BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm -rf  ${BUILD_DIR}/*
cmake \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS} ${DEBUG}" \
	-DCMAKE_C_FLAGS="${CFLAGS} ${DEBUG}" \
	-DCMAKE_Fortran_FLAGS="${FFLAGS} ${DEBUG}" \
	-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fugaku.cmake \
	-DINSTALL_DIR=${PMLIB_DIR} \
	-Denable_OPENMP=yes \
	-Dwith_MPI=no \
	-Denable_Fortran=yes \
	-Dwith_example=yes \
	-Dwith_PAPI=${PAPI_DIR} \
	-Dwith_OTF=${OTF_DIR} \
	..

make VERBOSE=1

make install

# 2. MPI version

BUILD_DIR=${PACKAGE_DIR}/BUILD_MPI
mkdir -p $BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm -rf  ${BUILD_DIR}/*
cmake \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS} ${DEBUG}" \
	-DCMAKE_C_FLAGS="${CFLAGS} ${DEBUG}" \
	-DCMAKE_Fortran_FLAGS="${FFLAGS} ${DEBUG}" \
	-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fugaku.cmake \
	-DINSTALL_DIR=${PMLIB_DIR} \
	-Denable_OPENMP=yes \
	-Dwith_MPI=yes \
	-Denable_Fortran=yes \
	-Dwith_example=yes \
	-Dwith_PAPI=${PAPI_DIR} \
	-Dwith_OTF=${OTF_DIR} \
	..

make VERBOSE=1
make install

