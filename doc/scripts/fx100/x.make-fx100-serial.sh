#! /bin/bash
set -x

INSTALL_DIR=${HOME}/pmlib/install_develop

SRC_DIR=${HOME}/pmlib/PMlib
BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean # >/dev/null 2>&1

CXXFLAGS="-Kopenmp,fast -Ntl_notrt -Nfjcex -w"
CFLAGS="-Kopenmp,fast -Ntl_notrt -w"
#	CFLAGS="-Xg -std=c99 -Kopenmp,fast -Ntl_notrt -w"
FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt -Knooptmsg"

# CFLAGS+="   -g -DDEBUG_PRINT_MONITOR -DDEBUG_PRINT_WATCH -DDEBUG_PRINT_OTF"
# CXXFLAGS+=" -g -DDEBUG_PRINT_MONITOR -DDEBUG_PRINT_WATCH -DDEBUG_PRINT_OTF"
# CFLAGS+="   -DDEBUG_PRINT_PAPI"
# CXXFLAGS+=" -DDEBUG_PRINT_PAPI -DDEBUG_PRINT_PAPI_THREADS -DDEBUG_PRINT_LABEL"

#	PAPI library for FX100 system is under /opt/FJSVXosDevkit/sparc64fx/rfs
#	which is automatically identified thru --with-papi=yes option.

../configure \
--prefix=${INSTALL_DIR} \
--with-comp=FJ \
--with-papi=yes \
--with-example=yes \
--host=sparc64-unknown-linux-gnu \
CXX=FCCpx CXXFLAGS="${CXXFLAGS}" \
CC=fccpx CFLAGS="${CFLAGS}" \
FC=frtpx FCFLAGS="${FCFLAGS}"

make
make install

