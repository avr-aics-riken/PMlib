#!/bin/bash
#PJM -N CHECK-PMLIB-BAND
#PJM --rsc-list "rscunit=rscunit_ft02"
#PJM --rsc-list "rscgrp=dv1-10"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:1:00"
#PJM --mpi "proc=4"
#PJM -j
#PJM -S

module load lang
module list
set -x
xospastop

date

hostname
grep "model name" /proc/cpuinfo| sort|uniq
#	HERE=$PWD
HERE=${HOME}/pmlib/package/BUILD_DIR/example
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

NPROCS=4
export OMP_NUM_THREADS=12

#	export HWPC_CHOOSER=USER
#	export HWPC_CHOOSER=FLOPS
export HWPC_CHOOSER=BANDWIDTH
#	export HWPC_CHOOSER=VECTOR
#	export HWPC_CHOOSER=CACHE
#	export HWPC_CHOOSER=LOADSTORE

mpiexec -n ${NPROCS} ./example1

