#!/bin/bash
#PJM -N DEBUG-FLOPS
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
DIR_SERIAL=${HOME}/pmlib/package/BUILD_DEBUG_SERIAL/example
HERE=${DIR_SERIAL}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
export OMP_NUM_THREADS=12
export HWPC_CHOOSER=FLOPS

xospastop
./example1
ldd ./example1
