#!/bin/bash
#PJM -N RUN-MPI
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=eap-small"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:10:00"
#PJM --mpi "max-proc-per-node=4"
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
rm stdout.*.txt

NPROCS=4
export OMP_NUM_THREADS=12

for i in FLOPS BANDWIDTH VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
mpiexec -std stdout.${i}.txt -np ${NPROCS} ${DIR_MPI}/example1
cat stdout.${i}.txt
done

rm stdout.*.txt

