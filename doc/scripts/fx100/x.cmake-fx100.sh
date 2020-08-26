#!/bin/bash
set -x

PACKAGE_DIR=${HOME}/pmlib/package
#	PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fx100
#	PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-6.4.0-fx100
#	PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-7.0.0-fx100
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-7.2.0-fx100

PAPI_DIR="yes"
#	PAPI_DIR=/opt/FJSVXosDevkit/sparc64fx/target/usr/lib64/
#	On K/FX100, PAPI library is automatically searched by mpi*px compilers.
#	So, setting "yes" is good enough to link PAPI.

OTF_DIR=${HOME}/otf/usr_local_otf/fx100

BUILD_DIR=${PACKAGE_DIR}/BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

CXXFLAGS="-DUSE_PRECISE_TIMER -Nfjcex -std=c++11 "
#	CXXFLAGS="-DUSE_PRECISE_TIMER -Nfjcex "
CFLAGS=" "
FFLAGS="-cpp "
DEBUG=" "

# 1. Serial version

rm -rf  ${BUILD_DIR}/*
cmake \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS} ${DEBUG}" \
	-DCMAKE_C_FLAGS="${CFLAGS} ${DEBUG}" \
	-DCMAKE_Fortran_FLAGS="${FFLAGS} ${DEBUG}" \
	-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fx100.cmake \
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

rm -rf  ${BUILD_DIR}/*
cmake \
	-DCMAKE_CXX_FLAGS="${CXXFLAGS} ${DEBUG}" \
	-DCMAKE_C_FLAGS="${CFLAGS} ${DEBUG}" \
	-DCMAKE_Fortran_FLAGS="${FFLAGS} ${DEBUG}" \
	-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fx100.cmake \
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

