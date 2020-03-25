#!/bin/bash
#PJM -N SINGLE-EA1
#	#PJM --rsc-list "rscunit=rscunit_ft02"
#	#PJM --rsc-list "rscgrp=dv1-10"
#PJM --rsc-list "rscunit=rscunit_ft01"
#PJM --rsc-list "rscgrp=dv27k"
#PJM --rsc-list "node=1"
#PJM --rsc-list "elapse=00:2:00"
#PJM -j
#PJM -S

#	module load lang/fjcompiler20200214_01
#	module load lang
module list
set -x

date
hostname

#	HERE=$PWD
#	HERE=${HOME}/pmlib/package/BUILD_DIR/example
#	DIR_SERIAL=${HOME}/pmlib/package/BUILD_SERIAL/example
DIR_SERIAL=${HOME}/pmlib/PMlib-ea1/BUILD_SERIAL/example
HERE=${DIR_SERIAL}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
export OMP_NUM_THREADS=12
export HWPC_CHOOSER=FLOPS
./example1
#	./example4

ldd ./example1

sleep 3
xospastop
sleep 3
./example1
ldd ./example1
