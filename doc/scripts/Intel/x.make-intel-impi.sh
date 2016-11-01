#!/bin/bash
# [1] Intel version

# Following autoconf path is needed only when configure.ac has been updated
AUTOCONF_PATH=${HOME}/autoconf/local_autoconf/bin
AUTOMAKE_PATH=${HOME}/autoconf/local_automake/bin
export PATH=${AUTOCONF_PATH}:${AUTOMAKE_PATH}:${PATH}

#	module load intel
INTEL_DIR=/ap/intel/composer_xe_2015.2.164
source ${INTEL_DIR}/bin/compilervars.sh intel64

#	module load impi
MPI_DIR=/ap/intel/impi/5.0.3.048
source ${MPI_DIR}/bin64/mpivars.sh
set -x

#	module load papi/intel
PAPI_DIR=${HOME}/papi/papi-intel
export LD_LIBRARY_PATH=${PAPI_DIR}/lib:${LD_LIBRARY_PATH}

INSTALL_DIR=${HOME}/pmlib/install_develop

SRC_DIR=${HOME}/pmlib/PMlib
BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean # >/dev/null 2>&1
#autoreconf -i

CFLAGS="-std=c99 -openmp"
FCFLAGS="-fpp -openmp "
CXXFLAGS="-openmp "

../configure \
	CXX=mpiicpc CC=mpiicc FC=mpiifort \
	CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" FCFLAGS="${FCFLAGS}" \
	-prefix=${INSTALL_DIR} \
	--with-comp=INTEL \
	--with-impi=${MPI_DIR} \
	--with-papi=${PAPI_DIR} \
	--with-example=yes # >/dev/null

make
exit
sleep 2
make install
if [ $? != 0 ] ; then echo '@@@ installation error @@@'; exit; fi
#	make distclean
#	exit
exit

# The following 2 lines can be used when compiling a user code with PAPI
#	export LDFLAGS+="-L${PAPI_DIR}/lib64 -L${PAPI_DIR}/lib -lpapi -lpfm"
#	export INCLUDES+="-I{PAPI_DIR}/include"
#
#   CXX=mpicxx CC=mpicc FC=mpif90 # GNU
#   CXX=mpiicpc CC=mpiicc FC=mpiifort # Intel
