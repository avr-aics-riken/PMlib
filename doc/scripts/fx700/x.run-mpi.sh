#! /bin/bash
	##!/bin/bash
	##PJM -N RUN-MPI
	##PJM --rsc-list "rscunit=rscunit_ft01"
	##PJM --rsc-list "rscgrp=dv41k"
	##PJM --rsc-list "node=1"
	##PJM --rsc-list "elapse=00:2:00"
	##PJM -j
	##PJM -S
set -x
export PATH=/opt/FJSVstclanga/v1.0.0/bin:$PATH
export LD_LIBRARY_PATH=/opt/FJSVstclanga/v1.0.0/lib64:$LD_LIBRARY_PATH
export LANG=C
date
hostname
xospastop

DIR_MPI=${HOME}/pmlib/package/BUILD_MPI/example
HERE=${DIR_MPI}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

NPROCS=2
export OMP_NUM_THREADS=12

for i in FLOPS BANDWIDTH VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
mpiexec -np ${NPROCS} ${DIR_MPI}/example1
done

ldd ${DIR_MPI}/example1

