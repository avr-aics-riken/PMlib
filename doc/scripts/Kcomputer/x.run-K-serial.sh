#!/bin/bash
#PJM -N PMLIB-USER-SERIAL
#PJM --rsc-list "elapse=1:00:00"
#PJM --rsc-list "node=1"
#PJM -j
#PJM -S
# stage io files
#PJM --stg-transfiles all
#PJM --stgin-basedir "/home/ra000004/a03155/tmp/check_pmlib"
#PJM --stgin "a.out.serial ./a.out.serial"


source /work/system/Env_base
set -x
date
hostname
/opt/FJSVXosPA/bin/xospastop

pwd
ls -l
export OTF_TRACING=full
export OTF_FILENAME="trace_pmlib"

export OMP_NUM_THREADS=2
./a.out.serial
ls -l
rm ${OTF_FILENAME}.*

export HWPC_CHOOSER=FLOPS
./a.out.serial
ls -l


