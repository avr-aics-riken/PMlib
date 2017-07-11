
#!/bin/bash
#	source /home/system/Env_base
MY_DIR=${HOME}/pmlib/my_dir
cd ${MY_DIR}; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
pwd
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

PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/K
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

PAPI_DIR="yes"
PAPI_LDFLAGS="-lpapi_ext -Wl,-Bstatic,-lpapi,-lpfm,-Bdynamic "

LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} -w -Nfjcex"

PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
INCLUDES="${PMLIB_INCLUDES} ${PAPI_INCLUDES} "

CXXFLAGS="-Kopenmp,fast -Ntl_notrt -w -DDISABLE_MPI "
LDFLAGS="${LDFLAGS}  --linkfortran"

set -x
FCCpx ${CXXFLAGS} ${INCLUDES} mxm.cpp ${LDFLAGS}

ls -l
file a.out


