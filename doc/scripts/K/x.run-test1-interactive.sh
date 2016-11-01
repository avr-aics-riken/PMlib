#! /bin/bash
source /work/system/Env_base
set -x
date
hostname
/opt/FJSVXosPA/bin/xospastop

cd ${HOME}/pmlib/PMlib/BUILD_DIR/example
pwd
#	export HWPC_CHOOSER=FLOPS
#	export HWPC_CHOOSER=BANDWIDTH
#	export HWPC_CHOOSER=VECTOR
#	export HWPC_CHOOSER=FLOPS,VECTOR,INSTRUCTION	# OK
#	export HWPC_CHOOSER=CYCLE,INSTRUCTION
#	export HWPC_CHOOSER=VECTOR	# OK
#	export HWPC_CHOOSER=CYCLE
#	export HWPC_CHOOSER=CACHE
#	export HWPC_CHOOSER=INSTRUCTION	# this is not available on K
#	export HWPC_CHOOSER=FLOPS,VECTOR	# this causes nan

NPROCS=2
export OMP_NUM_THREADS=4
mpiexec -n ${NPROCS} test1/test1

