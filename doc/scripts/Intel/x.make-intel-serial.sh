#!/bin/bash
# [1] Intel version
# Following autoconf path is needed only when configure.ac has been updated
AUTOCONF_PATH=${HOME}/autoconf/local_autoconf/bin
AUTOMAKE_PATH=${HOME}/autoconf/local_automake/bin
export PATH=${AUTOCONF_PATH}:${AUTOMAKE_PATH}:${PATH}

#       module load intel
INTEL_DIR=/ap/intel/composer_xe_2015.2.164
source ${INTEL_DIR}/bin/compilervars.sh intel64

#	#       module load impi
#	MPI_DIR=/ap/intel/impi/5.0.3.048
#	source ${MPI_DIR}/bin64/mpivars.sh
set -x

#       module load papi/intel
PAPI_DIR=${HOME}/papi/papi-intel
export LD_LIBRARY_PATH=${PAPI_DIR}/lib:${LD_LIBRARY_PATH}

INSTALL_DIR=${HOME}/pmlib/install_intel
#	INSTALL_DIR=/usr/local/pmlib/pmlib-5.1.1-intel

SRC_DIR=${HOME}/pmlib/PMlib
BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean >/dev/null 2>&1
#autoreconf -i

CFLAGS="-openmp "
FCFLAGS="-fpp -openmp "
CXXFLAGS="-openmp "

#   CFLAGS+=" -g   -DDEBUG_PRINT_MONITOR -DDEBUG_PRINT_WATCH -DDEBUG_PRINT_OTF"
#   CXXFLAGS+=" -g -DDEBUG_PRINT_MONITOR -DDEBUG_PRINT_WATCH -DDEBUG_PRINT_OTF"
#   CFLAGS+=" -DDEBUG_PRINT_PAPI -DDEBUG_PRINT_PAPI_THREADS -DDEBUG_PRINT_LABEL"
#   CXXFLAGS+=" -DDEBUG_PRINT_PAPI -DDEBUG_PRINT_PAPI_THREADS -DDEBUG_PRINT_LABEL"

../configure \
	CXX=icpc CC=icc FC=ifort \
	CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" FCFLAGS="${FCFLAGS}" \
	-prefix=${INSTALL_DIR} \
	--with-comp=INTEL \
	--with-example=yes \
	--with-papi=${PAPI_DIR} \
	# >/dev/null 2>&1

make
sleep 2
make install
if [ $? != 0 ] ; then echo '@@@ install error @@@'; exit; fi

sleep 2
example/test1/test1
sleep 2
export HWPC_CHOOSER=FLOPS
example/test1/test1
sleep 2
example/test2/test2
sleep 2
example/test3/test3	# MPI process group
sleep 2
example/test4/test4	# Fortran MPI
sleep 2
example/test5/test5	# MPI communicator

exit
