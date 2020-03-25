#!/bin/bash
#PJM -N CHECK-PMLIB-ALL
#	#PJM --rsc-list "rscunit=rscunit_ft02"
#	#PJM --rsc-list "rscgrp=dv1-10"
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=dv27k"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:10:00"
#PJM --mpi "proc=4"
#PJM -j
#PJM -S

#	module load lang
#	module load lang/fjcompiler20200214_01
module list
set -x
xospastop

date

hostname
grep "model name" /proc/cpuinfo| sort|uniq
DIR_SERIAL=${HOME}/pmlib/package/BUILD_SERIAL/example
cd ${DIR_SERIAL}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
export OMP_NUM_THREADS=12
for i in BANDWIDTH FLOPS VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
./example1
done

DIR_MPI=${HOME}/pmlib/package/BUILD_MPI/example
cd ${DIR_MPI}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
NPROCS=4
export OMP_NUM_THREADS=12

for i in BANDWIDTH FLOPS VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
mpiexec -n ${NPROCS} ./example1
done
