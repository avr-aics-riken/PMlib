#!/bin/bash
#PJM -N MYTEST1
#PJM --rsc-list "elapse=1:00:00"
#PJM --rsc-list "node=1"
#PJM --mpi "proc=2"
#PJM --mpi "rank-map-bychip"
#PJM -j
#PJM -S
# stage io files
#PJM --stg-transfiles all
#PJM --mpi "use-rankdir"
#PJM --stgin-basedir "/home/ra000004/a03155/pmlib/PMlib/BUILD_DIR/example"
#PJM --stgin "rank=* test1/test1 %r:./test1"
#
# to execute interactively on K compute nodes
#	pjsub --interact --rsc-list "elapse=01:00:00" --rsc-list "node=1" --mpi "proc=2"
source /work/system/Env_base
set -x
date
hostname
/opt/FJSVXosPA/bin/xospastop

pwd
#	export HWPC_CHOOSER=FLOPS
#	export HWPC_CHOOSER=VECTOR
#	export HWPC_CHOOSER=FLOPS,VECTOR,INSTRUCTION	# OK
#	export HWPC_CHOOSER=BANDWIDTH
#	export HWPC_CHOOSER=CYCLE,INSTRUCTION

NPROCS=2
export OMP_NUM_THREADS=4
mpiexec -n ${NPROCS} ./test1

