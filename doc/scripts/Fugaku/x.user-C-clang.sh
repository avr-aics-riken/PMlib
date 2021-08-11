#!/bin/bash
#PJM -N USER-C-CLANG
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=small"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:05:00"
#PJM --mpi "max-proc-per-node=8"
#PJM -j
#	#PJM -S
module list
date
hostname
export LANG=C
echo job manager reserved the following resources
echo PJM_NODE         : ${PJM_NODE}
echo PJM_MPI_PROC     : ${PJM_MPI_PROC}
echo PJM_PROC_BY_NODE : ${PJM_PROC_BY_NODE}
set -x

WKDIR=${HOME}/tmp/check_pmlib_C
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/* >/dev/null 2>&1


# on Login node
#	PAPI_DIR=/opt/FJSVxos/devkit/aarch64/rfs/usr
# on compute node
PAPI_DIR=/usr
PAPI_LDFLAGS="-L${PAPI_DIR}/lib64 -lpapi -lpfm "
PAPI_INCLUDES="-I${PAPI_DIR}/include "

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fugaku-develop
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "	# needed for C++ and C programs
# Choose MPI or serial
#	PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi -lpapi_ext -lpower_ext " # (1) MPI
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM -lpapi_ext -lpower_ext "	# (2) serial

MPI_LDFLAGS="-lmpi "

POWER_DIR="/opt/FJSVtcs/pwrm/aarch64"
POWER_LDFLAGS="-L${POWER_DIR}/lib64 -lpwr "
POWER_INCLUDES="-I${POWER_DIR}/include "

INCLUDES="${PAPI_INCLUDES} ${POWER_INCLUDES} ${PMLIB_INCLUDES} "
LDFLAGS="${PAPI_LDFLAGS} ${POWER_LDFLAGS} ${PMLIB_LDFLAGS} ${MPI_LDFLAGS} -lstdc++ "

FFLAGS="-Ofast -Kopenmp -w -Cpp ${DEBUG} "
CXXFLAGS="-Nclang --std=c++11 -Ofast -Kopenmp -w ${DEBUG} "
CFLAGS="-Nclang --std=c11 -Ofast -Kopenmp -w ${DEBUG} "

#	SRCDIR=${HOME}/pmlib/doc/src_tutorial
SRCDIR=${HOME}/pmlib/src_tests/src_test_others


cp $SRCDIR/main_mxm.c main.c
fcc ${CFLAGS} ${INCLUDES}	main.c  ${LDFLAGS}

xospastop
export OMP_NUM_THREADS=12
export HWPC_CHOOSER=FLOPS
export PMLIB_REPORT=FULL
export POWER_CHOOSER=NODES
./a.out

