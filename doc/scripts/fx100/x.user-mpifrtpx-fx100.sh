#! /bin/bash
#	module unload TCSuite
#	module load TCSuite/GM-2.0.0-03
set -x

PACKAGE_DIR=${HOME}/pmlib/package

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-fx100
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi "

PAPI_DIR=""
PAPI_LDFLAGS=" -lpapi_ext -lpapi -lpfm "

OTF_DIR=${HOME}/otf/local
OTF_LDFLAGS=" -lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

FX100LDFLAGS="-Ntl_notrt -Nfjcex"
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


#	MY_SRC=${HOME}/pmlib/PMlib-develop/example/test4
#	MY_SRC=${PACKAGE_DIR}/doc/src_tutorial
MY_SRC=${PACKAGE_DIR}/example/test4

cp $MY_SRC/f_main.f90 main.f90
#	cp $MY_SRC/main_thread.f90 main.f90
#	cp $MY_SRC/threadprivate.f90 main.f90

CXXFLAGS="-std=c++11 -Kopenmp -w -Nfjcex "
#	CXXFLAGS="-std=c++11 -Kopenmp -Ntl_notrt -w -Nfjcex "
CFLAGS=""
FCFLAGS="-Cpp -Ccpp -Kopenmp -Nrt_notune "

#	mpifrtpx does not work well with C++
#	so, link step must be done using C++ wrapper
mpifrtpx    ${FCFLAGS} ${INCLUDES} -c main.f90 
#	LDFLAGS+=" -lmpi_cxx -lstdc++ -lstd_mt -lfjcex "
LDFLAGS+=" -Xg --linkfortran "	# These are must options for this special linkage
mpiFCCpx    ${CXXFLAGS} main.o -o a.out ${INCLUDES}  ${LDFLAGS}

file a.out

