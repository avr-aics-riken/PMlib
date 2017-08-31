#! /bin/bash
module unload TCSuite
module load TCSuite/GM-2.0.0-03
set -x

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM -lpapi_ext "

PAPI_DIR=""
PAPI_LDFLAGS="-lpapi -lpfm "

OTF_DIR=${HOME}/otf/local
OTF_LDFLAGS=" -lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

FX100LDFLAGS="-Ntl_notrt"
LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS} ${FX100LDFLAGS} "


PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PAPI_INCLUDES=""
# Set FX100_INCLUDES for Fortran programs only
# FX100_INCLUDES="-I/opt/FJSVXosDevkit/sparc64fx/rfs/usr/include "
FX100_INCLUDES=""
INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES}"


WK_DIR=${HOME}/work/check_pmlib
mkdir -p $WK_DIR
cd $WK_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WK_DIR/*

PACKAGE_DIR=${HOME}/pmlib/package
cp ${PACKAGE_DIR}/doc/src_tutorial/main_serial.cpp  main.cpp


CXXFLAGS="-Kopenmp,fast -Ntl_notrt  -w -DDISABLE_MPI"
CFLAGS="-std=c99 -Kopenmp,fast -Ntl_notrt -w -DDISABLE_MPI"
FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt -DDISABLE_MPI"

FCCpx    ${CXXFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}

ls -go
file a.out
