#!/bin/bash
#PJM -N PMLIB-EXAMPLES
#PJM --rsc-list "elapse=1:00:00"
#PJM --rsc-list "node=4"
#PJM --mpi "proc=4"
#PJM --mpi "rank-map-bychip"
#PJM -j
#PJM -S
# stage io files
#PJM --stg-transfiles all
#PJM --mpi "use-rankdir"
#PJM --stgin-basedir "/home/ra000004/a03155/pmlib/package/BUILD_DIR/example"
#PJM --stgin "rank=* example1 %r:./example1"
#PJM --stgin "rank=* example2 %r:./example2"
#PJM --stgin "rank=* example3 %r:./example3"
#PJM --stgin "rank=* example4 %r:./example4"
#PJM --stgin "rank=* example5 %r:./example5"
#
#	source /home/system/Env_base
source /work/system/Env_base
set -x
date
hostname
/opt/FJSVXosPA/bin/xospastop

pwd
ls -l

NPROCS=4
export OMP_NUM_THREADS=8
mpiexec -n ${NPROCS} ./example1
mpiexec -n ${NPROCS} ./example2
mpiexec -n ${NPROCS} ./example3
mpiexec -n ${NPROCS} ./example4
mpiexec -n ${NPROCS} ./example5

export HWPC_CHOOSER=FLOPS
mpiexec -n ${NPROCS} ./example1
mpiexec -n ${NPROCS} ./example2
mpiexec -n ${NPROCS} ./example3
mpiexec -n ${NPROCS} ./example4
mpiexec -n ${NPROCS} ./example5

export HWPC_CHOOSER=BANDWIDTH
mpiexec -n ${NPROCS} ./example1

export HWPC_CHOOSER=CYCLE
mpiexec -n ${NPROCS} ./example1

export HWPC_CHOOSER=INSTRUCTION
mpiexec -n ${NPROCS} ./example1

export HWPC_CHOOSER=CACHE
mpiexec -n ${NPROCS} ./example1

