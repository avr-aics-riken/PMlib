#!/bin/bash
source /home/system/Env_base
set -x
date
hostname
#	/opt/FJSVXosPA/bin/xospastop

INSTALL_DIR=${HOME}/pmlib/install_K
SRC_DIR=${HOME}/pmlib/PMlib
BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean 2>&1 >/dev/null

CXXFLAGS="-Kopenmp,fast -Ntl_notrt -Nfjcex -DK_COMPUTER"
CFLAGS="-std=c99 -Kopenmp,fast -Ntl_notrt -w"
FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt -w -Knooptmsg"

if [[ $HOSTNAME =~ fe01p ]]
then
# K login node
../configure CXX=FCCpx CC=fccpx FC=frtpx \
   CXXFLAGS="${CXXFLAGS}" CFLAGS="${CFLAGS}" FCFLAGS="${FCFLAGS}" \
   --with-comp=FJ \
   --with-papi=yes \
   --with-example=yes \
   --host=sparc64-unknown-linux-gnu \
   --prefix=${INSTALL_DIR}

else
# compute node
../configure CXX=FCC CC=fcc FC=frt \
   CXXFLAGS="${CXXFLAGS}" CFLAGS="${CFLAGS}" FCFLAGS="${FCFLAGS}" \
   --with-comp=FJ \
   --with-papi=yes \
   --with-example=yes \
   --prefix=${INSTALL_DIR}
fi

make
#	exit

# on compute node, the flags should set for PAPI
#	PAPI_LIB="-L/usr/lib64 -lpapi -lpfm "
#	PAPI_INCLUDE=""


make install
