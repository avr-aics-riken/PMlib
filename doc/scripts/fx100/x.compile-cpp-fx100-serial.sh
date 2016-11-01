#! /bin/bash
set -x

PMLIB_DIR=${HOME}/pmlib/install_develop
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM -lpapi_ext "
#   京コンピュータでは計算ノード用PAPIは/lib64以下にインストールされている
PAPI_DIR=/
PAPI_INCLUDES="-I/include "
PAPI_LDFLAGS="-L/lib64 -lpapi -lpfm "

INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES}"
LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS}"

WK_DIR=${HOME}/work/check_pmlib
mkdir -p $WK_DIR
cd $WK_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WK_DIR/*

MY_SRC=${HOME}/pmlib/scripts/src_tests
cp $MY_SRC/mxm.cpp main.cpp

CXXFLAGS="-Kopenmp,fast -Ntl_notrt  -w -Nfjcex -D_PM_WITHOUT_MPI_"
CFLAGS="-std=c99 -Kopenmp,fast -Ntl_notrt -w"
FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt -w"

FCCpx    ${CXXFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}



