#!/bin/bash
#BSUB -J PMLIB-USER
#BSUB -o PMLIB-USER-%J
#BSUB -n 4
#BSUB -R "span[ptile=2]"
#BSUB -L /bin/bash
#BSUB -x
#	source  /usr/share/Modules/3.2.10/init/bash
#	module purge
#	module load intelcompiler/15.0.2.164 intelmpi/5.0.3
#	module load ~/local_module/pmlib/develop
#	module list

date
hostname
# Following autoconf path is needed only when configure.ac has been updated
AUTOCONF_PATH=${HOME}/autoconf/local_autoconf/bin
AUTOMAKE_PATH=${HOME}/autoconf/local_automake/bin
export PATH=${AUTOCONF_PATH}:${AUTOMAKE_PATH}:${PATH}

#       module load intel
INTEL_DIR=/ap/intel/composer_xe_2015.2.164
source ${INTEL_DIR}/bin/compilervars.sh intel64

#       module load impi
#	MPI_DIR=/ap/intel/impi/5.0.3.048
#	source ${MPI_DIR}/bin64/mpivars.sh

#	PMLIB_DIR=${HOME}/pmlib/install_develop
PMLIB_DIR=${HOME}/pmlib/install_intel

#	PAPI_DIR=${HOME}/papi/papi-intel
PAPI_DIR=/ap/papi/papi-5.4.1-intel
#	OTF_DIR=${HOME}/otf/otf-1.12-intel # OTF is not installed on eagles, yet

export PATH+=":${PAPI_DIR}/bin"

PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM -lpapi_ext "
PAPI_LDFLAGS="-L${PAPI_DIR}/lib64 -L${PAPI_DIR}/lib -lpapi -lpfm "
#	OTF_LDFLAGS="-L${PMLIB_DIR}/lib -lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS}"
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PAPI_DIR}/lib:${OTF_DIR}/lib

PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PAPI_INCLUDES="-I${PAPI_DIR}/include "
INCLUDES+=" ${PMLIB_INCLUDES} ${PAPI_INCLUDES}"

set -x

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/*

SRC_DIR=${HOME}/pmlib/scripts/src_tests
cp $SRC_DIR/mxm.cpp main.cpp
CXXFLAGS="-fopenmp ${REPORTS} -DDISABLE_MPI"
c++    ${CXXFLAGS} ${INCLUDES} main.cpp ${LDFLAGS}

#	export HWPC_CHOOSER=USER
export HWPC_CHOOSER=FLOPS
#	export HWPC_CHOOSER=VECTOR
#	export HWPC_CHOOSER=BANDWIDTH
#	export HWPC_CHOOSER=FLOPS
export OMP_NUM_THREADS=8
#	export OTF_TRACING=full
#	export OTF_FILENAME="xtra_otf_files"

echo starting main
sleep 2
./a.out
pwd
ls -go
