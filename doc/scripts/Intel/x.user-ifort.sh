#!/bin/bash
INTEL_TOP=/opt/intel
source ${INTEL_TOP}/bin/compilervars.sh intel64
set -x
umask 0022

# module load pmlib
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/intel
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

# module load papi
PAPI_DIR=${HOME}/papi/usr_local_papi/papi-5.5.1-intel
PAPI_LDFLAGS="-lpapi_ext -L${PAPI_DIR}/lib -Wl,-Bstatic,-lpapi,-lpfm,-Bdynamic "

# module load otf
#	OTF_DIR=${HOME}/otf/usr_local_otf/otf-1.12-intel
#	OTF_LDFLAGS="-lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS} -lstdc++ "

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/*

PACKAGE_DIR=${HOME}/pmlib/package
cp ${PACKAGE_DIR}/doc/src_tutorial/f_serial.f90 serial.f90

FCFLAGS="-O3 -qopenmp -fpp "
ifort    ${FCFLAGS} ${INCLUDES} serial.f90 ${LDFLAGS}

export OMP_NUM_THREADS=2
export HWPC_CHOOSER=FLOPS
export OTF_TRACING=full
export OTF_FILENAME="trace_check"

./a.out

