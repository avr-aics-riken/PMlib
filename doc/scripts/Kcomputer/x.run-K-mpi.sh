#!/bin/bash
#PJM -N PMLIB-USER-MPI
#PJM --rsc-list "elapse=1:00:00"
#PJM --rsc-list "node=1"
#PJM --mpi "proc=4"
#PJM --mpi "rank-map-bychip"
#PJM -j
#PJM -S
# stage io files
#PJM --stg-transfiles all
#PJM --mpi "use-rankdir"
#PJM --stgin-basedir "/home/ra000004/a03155/tmp/check_pmlib"
#PJM --stgin "rank=* a.out.mpi %r:./a.out"
#PJM --stgout-basedir "/home/ra000004/a03155/pmlib/scripts/dir_stgout"
#PJM --stgout "rank=*  %r:./trace_pmlib.*.events ./"
#PJM --stgout "rank=0  %r:./trace_pmlib.otf ./"
#PJM --stgout "rank=0  %r:./trace_pmlib.0.def ./"
#
source /work/system/Env_base
set -x
date
hostname
/opt/FJSVXosPA/bin/xospastop

pwd
ls -l
export OTF_TRACING=full
export OTF_FILENAME="trace_pmlib"

NPROCS=4
export OMP_NUM_THREADS=2
mpiexec -n ${NPROCS} ./a.out
ls -l
rm ${OTF_FILENAME}.*

export HWPC_CHOOSER=FLOPS
mpiexec -n ${NPROCS} ./a.out
ls -l


