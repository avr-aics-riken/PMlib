##############################################################################
#
# PMlib - Performance Monitor library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2013 Advanced Institute for Computational Science, RIKEN.
# All rights reserved.
#
##############################################################################
`#
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([PMlib], [1.9.3], [keno@riken.jp],[PMlib])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/PerfMonitor.cpp])
AC_CONFIG_HEADERS([config.h])


# revision No.
PM_REVISION="20130627_1400"
AC_SUBST(PM_REVISION)


#
# default prefix setting
#
AC_PREFIX_DEFAULT([/usr/local/PMlib])
if test "$prefix" = "NONE" ; then
  PM_INST_DIR=/usr/local/PMlib
else
  PM_INST_DIR="$prefix"
fi



#
# platform
#
PL_DEVICE="$($SHELL ./pm-uname)"
AC_SUBST(PL_DEVICE)

#
# MPI environment
#
enable_mpi="none"

AC_SUBST(MPI_DIR)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_LDFLAGS)
AC_SUBST(MPI_LIBS)


# mpich
AC_ARG_WITH(mpich, [AC_HELP_STRING([--with-mpich=dir],[Specify MPICH install directory])], , with_mpich=none)
if test "$with_mpich" != "none" ; then
  MPI_DIR=$with_mpich;
  MPI_CFLAGS=-I$MPI_DIR/include
  MPI_LDFLAGS=-L$MPI_DIR/lib
  MPI_LIBS=-lmpich
  enable_mpi="mpich"
fi


# openmpi
if test "$with_mpich" = "none" ; then
  AC_ARG_WITH( ompi, [AC_HELP_STRING([--with-ompi=dir],[Specify OpenMPI install directory])], , with_ompi=none)
  if test "$with_ompi" != "none" ; then
    MPI_DIR=$with_ompi;
    MPI_CFLAGS=-I$MPI_DIR/include
    MPI_LDFLAGS=-L$MPI_DIR/lib
    MPI_LIBS=-lmpi
    enable_mpi="ompi"
  fi
fi

if test "$enable_mpi" = "none" ; then
  echo "  Warning: Neither MPICH nor OpenMPI library is not specified."
  echo "           If compilation error occurs about MPI, specify --with-mpich or --with-ompi option."
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

PM_CXX="$CXX"
AC_SUBST(PM_CXX)


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
		])
AC_OUTPUT

chmod +x ./pm-config

