#! /bin/bash
set -x

PMLIB_DIR=${HOME}/pmlib/install_develop
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi -lpapi_ext "
PAPI_LDFLAGS="-lpapi -lpfm "

INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES}"
LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS}"

WK_DIR=${HOME}/work/check_pmlib
mkdir -p $WK_DIR
cd $WK_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WK_DIR/*

#	MY_SRC=${HOME}/pmlib/scripts/src_tests
#	cp $MY_SRC/mxm.cpp main.cpp
MY_SRC=${HOME}/pmlib/PMlib/doc/src_tutorial/
cp $MY_SRC/mpi_pmlib.cpp main.cpp

CXXFLAGS="-Kopenmp,fast -Ntl_notrt  -w -Nfjcex "
CFLAGS="-std=c99 -Kopenmp,fast -Ntl_notrt -w"
FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt -w"

mpiFCCpx    ${CXXFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}

ls -go
file a.out



