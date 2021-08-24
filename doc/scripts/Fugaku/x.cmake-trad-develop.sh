#!/bin/bash
set -x
export LANG=C

#	PACKAGE_DIR=${HOME}/pmlib/package
#	PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fugaku-libomp
#	PACKAGE_DIR=${HOME}/pmlib/PMlib-power
#	PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fugaku-power
#	PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fugaku-debug
PACKAGE_DIR=${HOME}/pmlib/PMlib-develop
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fugaku-trad

#	PAPI_DIR="no"
#	PAPI_DIR="yes"	# doesn't work on fugaku
#	PAPI libraries are on
# location on login nodes : /opt/FJSVxos/devkit/aarch64/rfs/usr/lib64/libpapi.so
# location on compute nodes : /usr/lib64/libpapi.so
PAPI_DIR=/opt/FJSVxos/devkit/aarch64/rfs/usr	# login node

#	Power API libraries are on
# location on login nodes : /opt/FJSVtcs/pwrm/aarch64/lib64/libpwr.so
# location on compute nodes : /usr/lib64/libpwr.so
#	POWER_DIR=/opt/FJSVtcs/pwrm/arrch64		# login node
POWER_DIR="yes"

#	On K/FX100, PAPI library is automatically searched by mpi*px compilers.
#	So, setting "yes" is good enough to link PAPI.

OTF_DIR="no"
#	OTF_DIR=${HOME}/otf/usr_local_otf/aarch64


#	CXXFLAGS="-DUSE_PRECISE_TIMER -Nfjcex "
#	CXXFLAGS="-Kocl "
CXXFLAGS="--std=c++11 -Kocl -Nnofjprof "
CFLAGS="--std=c11 -Kocl -Nnofjprof "
FFLAGS="-Kocl -Nnofjprof "

#	DEBUG="-DDEBUG_PRINT_MONITOR -DDEBUG_PRINT_WATCH -DDEBUG_PRINT_POWER_EXT -g "
#	DEBUG+="-DDEBUG_PRINT_PAPI -DDEBUG_PRINT_PAPI_THREADS "


# 1. Serial version

BUILD_DIR=${PACKAGE_DIR}/BUILD_TRAD_SERIAL
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
	-Dwith_POWER=${POWER_DIR} \
	-Dwith_OTF=${OTF_DIR} \
	..


make VERBOSE=1

make install

# 2. MPI version

BUILD_DIR=${PACKAGE_DIR}/BUILD_TRAD_MPI
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
	-Dwith_POWER=${POWER_DIR} \
	-Dwith_OTF=${OTF_DIR} \
	..

make VERBOSE=1
make install

