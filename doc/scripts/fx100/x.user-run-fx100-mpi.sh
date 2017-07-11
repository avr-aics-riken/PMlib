#! /bin/bash
#PJM -N PMLIB-USER-CXX
#PJM -L "elapse=1:00:00"
#PJM -L "node=1"
#PJM --mpi "proc=4"
#PJM -j
#PJM -S
set -x
xospastop

#	TEST_DIR=${HOME}/pmlib/PMlib/BUILD_DIR/example
TEST_DIR=${HOME}/work/check_pmlib
cd $TEST_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

#  export HWPC_CHOOSER=FLOPS
#  export HWPC_CHOOSER=BANDWIDTH
#  export HWPC_CHOOSER=VECTOR
#  export HWPC_CHOOSER=FLOPS,VECTOR,INSTRUCTION # OK
#  export HWPC_CHOOSER=CYCLE,INSTRUCTION
#  export HWPC_CHOOSER=VECTOR # OK
#  export HWPC_CHOOSER=CYCLE
#  export HWPC_CHOOSER=CACHE
#  export HWPC_CHOOSER=INSTRUCTION  # this is not available on K
#  export HWPC_CHOOSER=FLOPS,VECTOR # this causes nan

NPROCS=4
export OMP_NUM_THREADS=8
export HWPC_CHOOSER=FLOPS
#	export HWPC_CHOOSER=BANDWIDTH
#	export PAPI_PERFMON_EVENT_FILE=/usr/share/papi/papi_events.csv

mpiexec -n ${NPROCS} ./a.out
exit

