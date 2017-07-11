#!/bin/bash

MY_DIR=${HOME}/pmlib/my_dir
cd ${MY_DIR}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
if [ -f mxm.cpp ]
then
	echo "Compiling my program mxm.cpp in ${PWD}"
else
	echo "Oops... Can not find the source program mxm.cpp in ${PWD}"
	echo "Perhaps you can copy it from \$PACKAGE_DIR/doc/src_tutorial/"
	exit
fi
#	PACKAGE_DIR=${HOME}/pmlib/package
#	cp ${PACKAGE_DIR}/doc/src_tutorial/mxm.cpp  mxm.cpp
set -x

PMLIB_DIR=/usr/local/pmlib
PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

INCLUDES="${PMLIB_INCLUDES} "
LDFLAGS="${PMLIB_LDFLAGS} "

CXXFLAGS="-DDISABLE_MPI=true"
c++ ${CXXFLAGS} ${INCLUDES} -o main.ex mxm.cpp ${LDFLAGS}
export OMP_NUM_THREADS=1

./main.ex

