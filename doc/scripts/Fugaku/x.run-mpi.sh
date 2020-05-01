#!/bin/bash
#PJM -N RUN-MPI
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=dv41k"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:2:00"
#PJM --mpi "proc=4"
#PJM -j
#PJM -S

module list
set -x

date
hostname
xospastop

DIR_MPI=${HOME}/pmlib/package/BUILD_MPI/example
HERE=${DIR_MPI}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

NPROCS=4
export OMP_NUM_THREADS=12

for i in FLOPS BANDWIDTH VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
mpiexec -np ${NPROCS} ${DIR_MPI}/example1
done

ldd ${DIR_MPI}/example1

