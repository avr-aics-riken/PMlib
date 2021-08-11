#!/bin/bash
#PJM -N PRECISE-THREADS-STATIC
#	#PJM -N PRECISE-THREADS-DYNAMIC
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=small"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:02:00"
#PJM --mpi "max-proc-per-node=4"
#PJM -j
#	#PJM -S	# stats for each node
module list
date
hostname
export LANG=C
echo job manager reserved the following resources
echo PJM_NODE         : ${PJM_NODE}
echo PJM_MPI_PROC     : ${PJM_MPI_PROC}
echo PJM_PROC_BY_NODE : ${PJM_PROC_BY_NODE}
set -x

WKDIR=${HOME}/tmp/check_fort_precise
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/* >/dev/null 2>&1


# on Login node
#	PAPI_DIR=/opt/FJSVxos/devkit/aarch64/rfs/usr
# on compute node
PAPI_DIR=/usr
PAPI_LDFLAGS="-L${PAPI_DIR}/lib64 -lpapi -lpfm "
PAPI_INCLUDES="-I${PAPI_DIR}/include "

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/fugaku-precise


PMLIB_INCLUDES="-I${PMLIB_DIR}/include "	# needed for C++ and C programs
# Choose MPI (1) or serial (2)
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi -lpapi_ext -lpower_ext "	# (1) MPI
#	PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM -lpapi_ext -lpower_ext "	# (2) serial

MPI_LDFLAGS="-lmpi "

POWER_DIR="/opt/FJSVtcs/pwrm/aarch64"
POWER_LDFLAGS="-L${POWER_DIR}/lib64 -lpwr "
POWER_INCLUDES="-I${POWER_DIR}/include "

INCLUDES="${PAPI_INCLUDES} ${POWER_INCLUDES} ${PMLIB_INCLUDES} "
LDFLAGS="${PAPI_LDFLAGS} ${POWER_LDFLAGS} ${PMLIB_LDFLAGS} ${MPI_LDFLAGS} "
#	LDFLAGS+="--linkfortran "	# for C++ linker
#	LDFLAGS+="-lstdc++ "		# for fortran linker
LDFLAGS+="-lstdc++ "		# for fortran linker

FFLAGS="-Kopenmp -Cpp ${DEBUG} "
CXXFLAGS="-Nclang --std=c++11 -Kopenmp -w ${DEBUG} "
CFLAGS="-Nclang --std=c11 -Kopenmp -w ${DEBUG} "

#	SRCDIR=${PMLIB_DIR}/doc/src_tutorial
SRCDIR=${HOME}/pmlib/src_tests/src_test_threads

cp $SRCDIR/thread_pattern.f90 main.F90
mpifrt ${FFLAGS} main.F90 ${LDFLAGS}  # choose (1)

xospastop
export FLIB_FASTOMP=FALSE
export FLIB_CNTL_BARRIER_ERR=FALSE
export XOS_MMM_L_PAGING_POLICY=prepage:demand:demand

NPROCS=4
export OMP_NUM_THREADS=12

export HWPC_CHOOSER=FLOPS
export PMLIB_REPORT=FULL
export POWER_CHOOSER=NODE

mpiexec -std stdout.all.txt -np ${NPROCS} ./a.out
cat stdout.all.txt

