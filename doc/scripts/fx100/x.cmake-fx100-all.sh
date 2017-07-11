#! /bin/bash
module unload TCSuite
module load TCSuite/GM-2.0.0-03
set -x
cmake -version

PACKAGE_DIR=${HOME}/pmlib/package
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fx100
PAPI_DIR="yes"
#	PAPI_DIR=/opt/FJSVXosDevkit/sparc64fx/target/usr/lib64
#	On K/FX100, PAPI library is automatically searched by mpi*px compilers.
#	Setting "yes" is good enough to link PAPI.

OTF_DIR="no"
#	OTF_DIR=${HOME}/otf/usr_local_otf/fx100

BUILD_DIR=${PACKAGE_DIR}/BUILD_DIR
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

# 1. Serial version

rm -rf  ${BUILD_DIR}/*
cmake \
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
