
#!/bin/bash
#	source /home/system/Env_base
set -x

PACKAGE_DIR=${HOME}/pmlib/package
PMLIB_DIR=${HOME}/pmlib/usr_local_pmlib/K
PMLIB_LDFLAGS="-L${PMLIB_DIR}/lib -lPM "

PAPI_DIR="yes"
PAPI_LDFLAGS="-lpapi_ext -Wl,-Bstatic,-lpapi,-lpfm,-Bdynamic "

OTF_DIR=${HOME}/otf/usr_local_otf/opt-1.12.5
OTF_LDFLAGS="-lotf_ext -L${OTF_DIR}/lib -lopen-trace-format -lotfaux "

LDFLAGS+=" ${PMLIB_LDFLAGS} ${PAPI_LDFLAGS} ${OTF_LDFLAGS} -w -Nfjcex"

PMLIB_INCLUDES="-I${PMLIB_DIR}/include "
OTF_INCLUDES="-I${OTF_DIR}/include/open-trace-format "
INCLUDES="${PMLIB_INCLUDES} ${PAPI_INCLUDES} ${OTF_INCLUDES}"

WRK_DIR=${HOME}/tmp/check_pmlib
mkdir -p $WRK_DIR
cd $WRK_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
rm -f $WRK_DIR/*

cp ${PACKAGE_DIR}/doc/src_tutorial/f_serial.f90 serial.f90

#	add to FLAGS "-DDISABLE_MPI "

FCFLAGS="-Cpp -Kopenmp,fast -Ntl_notrt "
CXXFLAGS="-Kopenmp,fast -Ntl_notrt -w "
CFLAGS="-Kopenmp,fast -Ntl_notrt -w "
LDFLAGS="${LDFLAGS}  --linkfortran"

frtpx ${FCFLAGS} ${INCLUDES} -c serial.f90
FCCpx ${CXXFLAGS} ${INCLUDES} -o a.out.serial serial.o ${LDFLAGS}

ls -l
file a.out.serial


