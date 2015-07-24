#!/bin/bash
# [1] Intel version
module load gnu openmpi/gnu
module load papi/gnu-papi-5.3.2
module list
set -x
SRC_DIR=${HOME}/pmlib/PMlib-develop
cd $SRC_DIR; if [ $? != 0 ] ; then echo '@@@ Directory error @@@'; exit; fi
ldd example/pmlib_test
example/pmlib_test
