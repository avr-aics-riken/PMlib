#! /bin/bash
#PJM -N FORT-PMLIB
#PJM -L "elapse=0:10:00"
#PJM -L "node=1"
#PJM --mpi "proc=1"
#PJM -j
#PJM -S
set -x

PACKAGE_DIR=${HOME}/pmlib/package

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-fx100
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

PAPI_DIR=""
PAPI_LDFLAGS=" -lpapi_ext -lpapi -lpfm "

OTF_DIR=${HOME}/otf/usr_local_otf/fx100
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

#	cp ${HOME}/pmlib/scripts/src_tests/main_thread.f90 main.f90
cp ${PACKAGE_DIR}/doc/src_tutorial/mxm.f90  main.f90

FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt "
frt    ${FCFLAGS} ${INCLUDES} -o a.out main.f90  ${LDFLAGS} 

export HWPC_CHOOSER=FLOPS
#   export HWPC_CHOOSER=BANDWIDTH
#   export HWPC_CHOOSER=VECTOR
#   export HWPC_CHOOSER=CACHE
#   export HWPC_CHOOSER=CYCLE
#   export HWPC_CHOOSER=LOADSTORE
#   export HWPC_CHOOSER=USER

export OMP_NUM_THREADS=8

./a.out

