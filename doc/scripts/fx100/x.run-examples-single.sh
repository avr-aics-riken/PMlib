#! /bin/bash
#PJM -N FX100-SINGLE-EXAMPLES
#PJM -L "elapse=1:00:00"
#PJM -L "node=1"
#PJM --mpi "proc=1"
#PJM -j
#PJM -S

# Start batch session
#	~/x.interactive.sh
set -x
xospastop

TEST_DIR=${HOME}/pmlib/package/BUILD_DIR/example
cd $TEST_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

export OMP_NUM_THREADS=8
#	export OTF_TRACING=full
#	export OTF_FILENAME="trace_pmlib"

for HWPC_CHOOSER in BANDWIDTH FLOPS VECTOR CACHE CYCLE LOADSTORE USER
do
export HWPC_CHOOSER
./example4
done

