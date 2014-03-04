##############################################################################
#
# PMlib - Performance Monitor library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2014 Advanced Institute for Computational Science, RIKEN.
# All rights reserved.
#
##############################################################################
`#
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.57])
AC_INIT([PMlib], [2.0.1], [keno@riken.jp],[PMlib])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/PerfMonitor.cpp])
AC_CONFIG_HEADERS([config.h])


# revision No.
PM_REVISION="20140304_1230"
AC_SUBST(PM_REVISION)


#
# default prefix setting
#
AC_PREFIX_DEFAULT([/usr/local/PMlib])
if test x"$prefix" = x"NONE" ; then
  PM_INST_DIR=/usr/local/PMlib
else
  PM_INST_DIR="$prefix"
fi


#
# MPI environment
#
enable_mpi="none"


PM_CXX="$CXX"
AC_SUBST(PM_CXX)

case "$PM_CXX" in
  mpicxx)   enable_mpi="wrappermpi" ;;
  mpicc)    enable_mpi="wrappermpi" ;;
  mpiCC)    enable_mpi="wrappermpi" ;;
  mpic++)   enable_mpi="wrappermpi" ;;
  mpiFCCpx) enable_mpi="wrappermpi" ;;
  CC)       enable_mpi="wrappermpi" ;;
esac


AC_SUBST(MPI_DIR)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_LDFLAGS)
AC_SUBST(MPI_LIBS)


#
# if wrapper compiler is specified, skip this part
#

if test "$enable_mpi" = "none" ; then

# openmpi
  AC_ARG_WITH( ompi, [AC_HELP_STRING([--with-ompi=dir],[Specify OpenMPI install directory])], , with_ompi=none)
  if test x"$with_ompi" != x"none" ; then
    MPI_DIR=$with_ompi;
    MPI_CFLAGS=-I$MPI_DIR/include
    MPI_LDFLAGS=-L$MPI_DIR/lib
    MPI_LIBS=-lmpi
    enable_mpi="ompi"
  fi
fi

# warning. if a wrapper compiler is used, "enable_mpi" is empty, but parallel.
#

if test x"$enable_mpi" != x"wrappermpi" ; then
  if test x"$with_ompi" = x"none" ; then
    echo "  Warning: OpenMPI library is not specified."
    echo "           If compilation error occurs about MPI, specify --with-ompi option."
  fi
fi





#
# PM special flags
#
PM_CFLAGS="-I$PM_INST_DIR/include"
AC_SUBST(PM_CFLAGS)

PM_LDFLAGS="-L$PM_INST_DIR/lib"
AC_SUBST(PM_LDFLAGS)

PM_LIBS="-lPM"
AC_SUBST(PM_LIBS)



#
# MAKE_SUB_DIRS
#
AC_SUBST(MAKE_SUB_DIRS)
MAKE_SUB_DIRS="src doc"


#
# Checks for programs.
#
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.

#
# Checks for header files.
#
AC_CHECK_HEADERS([sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday sqrt])

AC_CONFIG_FILES([Makefile
                 src/Makefile \
                 doc/Makefile \
                 pm-config \
                 include/Version.h \
		])
AC_OUTPUT

chmod +x ./pm-config

