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
AC_INIT([PMlib], [1.9.1], [keno@riken.jp],[PMlib])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/PerfMonitor.cpp])
AC_CONFIG_HEADERS([config.h])


# revision No.
PM_REVISION="20130625_1300"
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
# compiler libs
#
AC_ARG_WITH(comp, [AC_HELP_STRING([--with-comp=(INTEL|FJ)],[Specify Compiler type])], , with_comp=none)
AC_SUBST(COMPILER_LIBS)
case "$with_comp" in
"INTEL")
  COMPILER_LIBS="-lifport -lifcore"
  ;;
"FJ")
  COMPILER_LIBS="--linkfortran"
  ;;
esac

enable_mpi="none"


#
# MPICH environment
#
AC_SUBST(MPICH_DIR)
AC_SUBST(MPICH_CFLAGS)
AC_SUBST(MPICH_LDFLAGS)
AC_SUBST(MPICH_LIBS)
AC_SUBST(MPICH_FCLIBS)
AC_ARG_WITH(mpich, [AC_HELP_STRING([--with-mpich=dir],[Specify MPICH install directory])], , with_mpich=none)
if test "$with_mpich" != "none" ; then
  MPICH_DIR=$with_mpich;
  MPICH_CFLAGS=-I$MPICH_DIR/include
  MPICH_LDFLAGS=-L$MPICH_DIR/lib
  enable_mpi="mpich"
fi

#
# OpenMPI environment
#
if test "$with_mpich" = "none" ; then
  AC_ARG_WITH(ompi, [AC_HELP_STRING([--with-ompi=dir],[Specify OpenMPI install directory])], , with_ompi=none)
  if test "$with_ompi" != "none" ; then
    MPICH_DIR=$with_ompi;
    MPICH_CFLAGS=-I$MPICH_DIR/include
    MPICH_LDFLAGS=-L$MPICH_DIR/lib
    enable_mpi="ompi"
  fi
fi


#
# MPI Library
#
if test "$with_comp" = "FJ" ; then
  MPICH_LIBS=""
  case "$host" in
  sparc*)
    MPICH_FCLIBS="$MPICH_LIBS"" -lmpi_f77 -lmpi_f90"
    ;;
  esac
elif test "$enable_mpi" = "mpich" ; then
  MPICH_LIBS="-lmpich"
  MPICH_FCLIBS="$MPICH_LIBS"" -lfmpich"
  case "$CPM_DEVICE" in
  "Lion"|"Snow_Leopard"|"Leopard"|"Mountain_Lion")
#    MPICH_LIBS="$MPICH_LIBS"" -lpmpich"
    ;;
  esac
elif test "$enable_mpi" = "ompi" ; then
  MPICH_LIBS="-lmpi"
  MPICH_FCLIBS="$MPICH_LIBS"" -lmpi_f77 -lmpi_f90"
  case "$CPM_DEVICE" in
  "Lion"|"Snow_Leopard"|"Leopard"|"Mountain_Lion")
#    MPICH_LIBS="$MPICH_LIBS"" -lmpi_cxx"
    ;;
  esac
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

