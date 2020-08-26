#! /bin/bash
	##!/bin/bash
	##PJM -N RUN-SINGLE
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

DIR_SERIAL=${HOME}/pmlib/package/BUILD_SERIAL/example
HERE=${DIR_SERIAL}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
export OMP_NUM_THREADS=12

for i in FLOPS BANDWIDTH VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER=${i}
${DIR_SERIAL}/example1
done

#	export BYPASS_PMLIB=yes
#	export HWPC_CHOOSER=USER
#	${DIR_SERIAL}/example1

