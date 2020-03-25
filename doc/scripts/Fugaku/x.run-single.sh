#!/bin/bash
#PJM -N RUN-SINGLE
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=dv41k"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:2:00"
#PJM -j
#PJM -S

module list
set -x

date
hostname
xospastop

#	HERE=$PWD
#	HERE=${HOME}/pmlib/package/BUILD_DIR/example
#	DIR_SERIAL=${HOME}/pmlib/package/BUILD_SERIAL/example
DIR_SERIAL=${HOME}/pmlib/package/BUILD_SERIAL/example
HERE=${DIR_SERIAL}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
export OMP_NUM_THREADS=12

for i in FLOPS BANDWIDTH VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
${DIR_SERIAL}/example1
done

ldd ${DIR_SERIAL}/example1

