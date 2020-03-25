#!/bin/bash
module load lang
export LANG=C
set -x

PACKAGE_DIR=${HOME}/pmlib/package
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/aarch64
PAPI_DIR=/opt/FJSVxos/devkit/aarch64/rfs/usr
PAPI_SRC=${HOME}/papi/papi-5.6.0/src

BUILD_DIR=${PACKAGE_DIR}/BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi


cd example
FCCpx -Nrt_notune -w -Kopenmp -DDISABLE_MPI CMakeFiles/example1.dir/test1/main_pmlib.cpp.o  -o example1  -L${BUILD_DIR}/src  -L${BUILD_DIR}/src_papi_ext  -L/opt/FJSVxos/devkit/aarch64/rfs/usr/lib64 -rdynamic -lPM -lpapi_ext -lpapi -lpfm \
${PAPI_SRC}/cpus.o \
${PAPI_SRC}/extras.o \
${PAPI_SRC}/papi*.o \
${PAPI_SRC}/threads.o \
${PAPI_SRC}/sw*.o  \
${PAPI_SRC}/linux*.o \
-L{PAPI_DIR}/lib64 -lpapi -lpfm 

