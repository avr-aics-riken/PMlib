#! /bin/bash
module unload TCSuite
module load TCSuite/GM-2.0.0-03
set -x

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi "

PAPI_DIR=""
PAPI_LDFLAGS=" -lpapi_ext -lpapi -lpfm "

OTF_DIR=${HOME}/otf/usr_local_otf/fx100
OTF_LDFLAGS=" -lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

FX100LDFLAGS="-Ntl_notrt"
LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS} ${FX100LDFLAGS} "


PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PAPI_INCLUDES=""
# Set FX100_INCLUDES for Fortran programs only
#	FX100_INCLUDES="-I/opt/FJSVXosDevkit/sparc64fx/rfs/usr/include "
FX100_INCLUDES=""
INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES} ${FX100_INCLUDES} "

WK_DIR=${HOME}/work/check_pmlib
mkdir -p $WK_DIR
cd $WK_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WK_DIR/*

PACKAGE_DIR=${HOME}/pmlib/package
cp ${PACKAGE_DIR}/doc/src_tutorial/main_mpi.cpp  main.cpp

#	CXXFLAGS="-Kopenmp,fast -Ntl_notrt  -w"
#	CFLAGS="-std=c99 -Kopenmp,fast -Ntl_notrt -w"
#	FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt -w"

CXXFLAGS="-Kopenmp,fast -w "
CFLAGS="-std=c99 -Kopenmp,fast -w "
FCFLAGS="-Cpp -Kopenmp,fast "

mpiFCCpx    ${CXXFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}

ls -go
file a.out
