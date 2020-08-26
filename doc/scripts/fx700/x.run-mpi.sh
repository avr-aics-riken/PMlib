#! /bin/bash
#	module load lang
#	module load lang/fjcompiler20200214_01
set -x
export PATH=/opt/FJSVstclanga/v1.0.0/bin:$PATH
export LD_LIBRARY_PATH=/opt/FJSVstclanga/v1.0.0/lib64:$LD_LIBRARY_PATH
export LANG=C
date
hostname
xospastop

DIR_MPI=${HOME}/pmlib/package/BUILD_MPI/example
HERE=${DIR_MPI}
cd ${HERE}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi

NPROCS=2
export OMP_NUM_THREADS=12

#	export HWPC_CHOOSER=USER
#	export BYPASS_PMLIB=yes

#	for i in FLOPS BANDWIDTH VECTOR CACHE CYCLE LOADSTORE USER
for i in USER
do
export HWPC_CHOOSER=${i}
mpiexec -np ${NPROCS} ${DIR_MPI}/example1
done

ldd ${DIR_MPI}/example1

