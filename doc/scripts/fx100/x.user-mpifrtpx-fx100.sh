#! /bin/bash
module unload TCSuite
module load TCSuite/GM-2.0.0-03
set -x

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi "

PAPI_DIR=""
PAPI_LDFLAGS=" -lpapi_ext -lpapi -lpfm "

OTF_DIR=${HOME}/otf/local
OTF_LDFLAGS=" -lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

#	FX100LDFLAGS="-Ntl_notrt"
FX100LDFLAGS="-Ntl_notrt --linkfortran "
LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS} ${FX100LDFLAGS} "


PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PAPI_INCLUDES=""
# Set FX100_INCLUDES for Fortran programs only
FX100_INCLUDES="-I/opt/FJSVXosDevkit/sparc64fx/rfs/usr/include "
INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES} ${FX100_INCLUDES} "

WK_DIR=${HOME}/work/check_pmlib
mkdir -p $WK_DIR
cd $WK_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WK_DIR/*


MY_SRC=${HOME}/pmlib/PMlib-5.6.3/example/test4
cp $MY_SRC/f_main.f90 main.f90

CXXFLAGS="-Kopenmp,fast -Ntl_notrt -w "
CFLAGS="-std=c99 -Kopenmp,fast -Ntl_notrt -w "
FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt "

#NG	mpifrtpx    ${FCFLAGS} ${INCLUDES} main.f90 ${LDFLAGS}

mpifrtpx    ${FCFLAGS} ${INCLUDES} -c main.f90
mpiFCCpx    ${CXXFLAGS} ${INCLUDES} main.o ${LDFLAGS}

ls -go
file a.out
