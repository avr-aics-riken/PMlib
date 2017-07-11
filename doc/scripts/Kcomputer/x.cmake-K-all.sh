#!/bin/bash
source /home/system/Env_base
set -x
date
hostname
export PATH=${HOME}/cmake/cmake-2.8.12.2/bin:/opt/local/bin:$PATH
cmake --version

PACKAGE_DIR=${HOME}/pmlib/package
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/K
PAPI_DIR="yes"
#	PAPI_DIR=/opt/FJSVXosDevkit/sparc64fx/target/usr/lib64
#	On K/FX100, PAPI library is automatically searched by mpi*px compilers.
#	Setting "yes" is good enough to link PAPI.

OTF_DIR="no"
#	OTF_DIR=${HOME}/otf/usr_local_otf/otf-1.12.5

BUILD_DIR=${PACKAGE_DIR}/BUILD_DIR
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

# 1. Serial version

rm -rf  ${BUILD_DIR}/*
cmake \
	-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_K.cmake \
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
	-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_K.cmake \
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

