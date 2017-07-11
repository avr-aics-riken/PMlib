#! /bin/bash

set -x
xospastop

# Start batch session
#	~/x.interactive.sh

#	TEST_DIR=${HOME}/pmlib/PMlib/BUILD_DIR/example
#	TEST_DIR=${HOME}/pmlib/PMlib-eagles/BUILD_DIR/example
#	TEST_DIR=${HOME}/pmlib/PMlib-5.6.2/BUILD_DIR/example
TEST_DIR=${HOME}/pmlib/PMlib-5.6.3/BUILD_DIR/example
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

export OMP_NUM_THREADS=2
export HWPC_CHOOSER=FLOPS

./example1

export HWPC_CHOOSER=BANDWIDTH
export OTF_TRACING=full
export OTF_FILENAME="trace_pmlib"

ls -l
./example1
ls -l

