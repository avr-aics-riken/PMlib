#!/bin/bash

MY_DIR=${HOME}/pmlib/my_dir
cd ${MY_DIR}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
pwd
if [ -f mxm.f90 ]
then
	echo "Compiling my program mxm.f90 in ${PWD}"
else
	echo "Oops... Can not find the source program mxm.f90 in ${PWD}"
	echo "Perhaps you can copy it from \$PACKAGE_DIR/doc/src_tutorial/"
	exit
fi

INTEL_DIR=/opt/intel
source ${INTEL_DIR}/bin/compilervars.sh intel64

#   module load pmlib
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-intel
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

#   module load papi
PAPI_DIR=${HOME}/papi/usr_local_papi/papi-5.6.0-intel
PAPI_LDFLAGS="-lpapi_ext -L${PAPI_DIR}/lib -Wl,-Bstatic,-lpapi,-lpfm,-Bdynamic "

LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} -lstdc++ "

PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PAPI_INCLUDES="-I${PAPI_DIR}/include "
INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES}"
set -x

FCFLAGS="-O3 -qopenmp -fpp "
ifort    ${FCFLAGS} ${INCLUDES} mxm.f90 ${LDFLAGS}

export OMP_NUM_THREADS=1
#	export HWPC_CHOOSER=FLOPS
./a.out
