#! /bin/bash
set -x

SRC_DIR=${HOME}/pmlib/PMlib
BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean # >/dev/null 2>&1

CXXFLAGS="-Kopenmp,fast -Ntl_notrt  -v -Nfjcex"
CFLAGS="-std=c99 -Kopenmp,fast -Ntl_notrt -w"
FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt -w"
INSTALL_DIR=${HOME}/pmlib/install_develop

../configure --prefix=${INSTALL_DIR} \
--with-comp=FJ \
--with-papi=yes \
--with-example=yes \
--host=sparc64-unknown-linux-gnu \
CXX=mpiFCCpx CXXFLAGS="${CXXFLAGS}" \
CC=mpifccpx CFLAGS="${CFLAGS}" \
FC=mpifrtpx FCFLAGS="${FCFLAGS}"

make
make install

