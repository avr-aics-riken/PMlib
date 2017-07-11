#!/bin/bash
set -x

PMLIB_DIR=/usr/local/pmlib
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

OTF_DIR=/usr/local/otf/otf-1.12.5-clang
OTF_LDFLAGS="-lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

GFORTRAN_LDFLAGS="-L/usr/local/gfortran/lib -lgfortran -lgomp -lc++ "
LDFLAGS="${PMLIB_LDFLAGS} ${OTF_LDFLAGS} ${GFORTRAN_LDFLAGS} "

INCLUDES="${PMLIB_INCLUDES} "

WKDIR=${HOME}/tmp/check_pmlib
mkdir -p $WKDIR
cd $WKDIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm $WKDIR/*

PACKAGE_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/package
cp ${PACKAGE_DIR}/doc/src_tutorial/f_serial.f90 serial.f90

FCFLAGS="-cpp -fopenmp"
gfortran ${FCFLAGS} ${INCLUDES} -o serial.ex serial.f90 ${LDFLAGS}
export OMP_NUM_THREADS=4
./serial.ex


#	2-step compile/link is possible
#Good!	gfortran ${FCFLAGS} ${INCLUDES} -c serial.f90
#Good!	c++ -o serial.ex serial.o ${LDFLAGS}
#Good!	./serial.ex
#	on Mac, these are not needed: "-lstdc++ -lstdc++fs -lsupc++ "
