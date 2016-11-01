#!/bin/bash
echo default PATH:; echo -n '	' # control+v and TAB key
echo $PATH | sed  '1,$s/:/:	/g' | tr ':' '\n'
set -x
date
hostname
#	/opt/local/bin/gcc == gcc 4.8.5 (MacPorts gcc48 4.8.5_0)
#	/usr/local/bin/gcc-5 == gcc-5 (Homebrew gcc 5.3.0)
#	/usr/bin/gcc == clang (Apple LLVM version 7.0.2)
#	/usr/local/openmpi/openmpi-1.8.7-gcc/bin/mpicc == clang 
#	/usr/local/openmpi/openmpi-1.8.7-clang/bin/mpicc == clang 

# default PATH: /usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:

export PATH=/opt/local/bin:/opt/local/sbin:${PATH}
gcc --version
g++ --version

INSTALL_DIR=${HOME}/Desktop/Programming/Perf_tools/AICS-PMlib/install_gnu
SRC_DIR=${HOME}/Desktop/Git_Repos/pmlib/mikami3heart/PMlib

BUILD_DIR=${SRC_DIR}/BUILD_DIR
cd $BUILD_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
make distclean >/dev/null 2>&1

#	do not set "-fopenmp" on my Mac, as the compilers are not built as so.
FCFLAGS="-cpp"	#	FCFLAGS="${FCFLAGS} -DDEBUG_FORT"
CFLAGS=""	#	CFLAGS="${CFLAGS} -DDEBUG_FORT"
CXXFLAGS=""	#	CXXFLAGS="${CXXFLAGS} -DDEBUG_FORT"

../configure \
	CXX=g++ CC=gcc FC=gfortran \
	CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" FCFLAGS="${FCFLAGS}" \
	--prefix=${INSTALL_DIR} \
	--with-comp=GNU \
	--with-example=yes \
	--with-papi=none \
	# >/dev/null 2>&1

make
gfortran --version

sleep 2
example/test1/test1
sleep 2
example/test2/test2
sleep 2
example/test3/test3
sleep 2
example/test4/test4

make install # may be done by root
if [ $? != 0 ] ; then echo '@@@ installation error @@@'; exit; fi
sleep 5

