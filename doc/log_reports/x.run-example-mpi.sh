#!/bin/bash
#PJM -N EXAMPLE-CLANG-MPI
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=small"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:30:00"
#PJM --mpi "max-proc-per-node=4"
#PJM -j
#PJM -S

module list
set -x

date
hostname
xospastop

DIR_MPI=${HOME}/pmlib/PMlib-develop/BUILD_CLANG_MPI/example
HERE=${DIR_MPI}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

#   export PMLIB_REPORT=BASIC
export PMLIB_REPORT=DETAIL
#   export PMLIB_REPORT=FULL

#   export POWER_CHOOSER=OFF
#   export POWER_CHOOSER=NODE
export POWER_CHOOSER=NUMA
#   export POWER_CHOOSER=PARTS

NPROCS=4
export OMP_NUM_THREADS=12

rm stdout.*.txt
for i in FLOPS BANDWIDTH VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
for j in 1 2 3 4 5
do
mpiexec -std stdout.${i}.txt -np ${NPROCS} ${DIR_MPI}/example${j}
cat stdout.${i}.txt
rm stdout.${i}.txt
done
done


