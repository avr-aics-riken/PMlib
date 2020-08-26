#!/bin/bash
set -x
umask 0022

# module load pmlib
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-5.7.0-gnu
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

# module load papi
PAPI_DIR=${HOME}/papi/usr_local_papi/papi-5.6.0-gnu
PAPI_LDFLAGS="-lpapi_ext -L${PAPI_DIR}/lib -Wl,-Bstatic,-lpapi,-lpfm,-Bdynamic "

LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS} -lstdc++ "

# Remark: the include files are needed for C++ , and not for fortran.
# But let's include them anyway.
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PAPI_INCLUDES="-I${PAPI_DIR}/include "
INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES}"

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/*

cp ${HOME}/pmlib/scripts/src_tests/f_serial.f90 serial.f90

FCFLAGS="-O3 -fopenmp -cpp "
gfortran    ${FCFLAGS} ${INCLUDES} serial.f90 ${LDFLAGS}

export OMP_NUM_THREADS=2
export HWPC_CHOOSER=FLOPS

./a.out
ls -go
