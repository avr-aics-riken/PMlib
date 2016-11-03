#!/bin/bash
echo default PATH:; echo -n '	' # control+v and TAB key
echo $PATH | sed  '1,$s/:/:	/g' | tr ':' '\n'
set -x
date
hostname
#	export PATH=/opt/local/bin:/opt/local/sbin:${PATH}
#	export PATH=${PATH}:/opt/local/bin:/opt/local/sbin

INSTALL_DIR=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_mac
SRC_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/PMlib

BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean >/dev/null 2>&1

FCFLAGS="-cpp"
CFLAGS=""
CXXFLAGS=""

#	FCFLAGS="${FCFLAGS} -DDEBUG_FORT"
#	CFLAGS="${CFLAGS} -DDEBUG_FORT -DDEBUG_PRINT_PAPI -DDEBUG_PRINT_LABEL"
#	CXXFLAGS="${CXXFLAGS} -DDEBUG_FORT -DDEBUG_PRINT_PAPI -DDEBUG_PRINT_LABEL -DDEBUG_PRINT_PAPI_THREADS"

../configure \
	CXX=c++ CC=cc FC=gfortran \
	CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" FCFLAGS="${FCFLAGS}" \
	--prefix=${INSTALL_DIR} \
	--with-papi=none \
	--with-example=yes

	#	CXX=clang++ CC=cc FC=gfortran \

make
sleep 2
gfortran --version
c++ --version
cc --version
make install # may be done by root
if [ $? != 0 ] ; then echo '@@@ installation error @@@'; exit; fi
exit

sleep 2
time example/test1/test1
sleep 2
time example/test2/test2
sleep 2
time example/test3/test3
sleep 2
time example/test4/test4
sleep 2
time example/test5/test5


