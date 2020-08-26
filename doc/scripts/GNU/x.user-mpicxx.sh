#!/bin/bash
set -x
umask 0022

#   OPENMPI_DIR=${HOME}/openmpi/openmpi-1.10.3
OPENMPI_DIR=${HOME}/openmpi/usr_local_openmpi
MPI_DIR=${OPENMPI_DIR}
export  PATH=${OPENMPI_DIR}/bin:${PATH}
export  LD_LIBRARY_PATH=${OPENMPI_DIR}/lib:${LD_LIBRARY_PATH}

# module load pmlib
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/pmlib-5.7.0-gnu
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPMmpi "

# module load papi
PAPI_DIR=${HOME}/papi/usr_local_papi/papi-5.6.0-gnu
PAPI_LDFLAGS="-lpapi_ext -L${PAPI_DIR}/lib -Wl,-Bstatic,-lpapi,-lpfm,-Bdynamic "

LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS} -lstdc++ "


# Remark: the include files are needed for C++ , and not for fortran.
# But let's include them anyway.
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PAPI_INCLUDES="-I${PAPI_DIR}/include "
INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES}"

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/*

cp ${HOME}/pmlib/scripts/src_tests/main_mpi.cpp main.cpp

FCFLAGS="-O3 -fopenmp -cpp "
mpicxx    ${FCFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}

export OMP_NUM_THREADS=2
export HWPC_CHOOSER=FLOPS

NPROCS=2
mpirun -np ${NPROCS} ./a.out
ls -go
