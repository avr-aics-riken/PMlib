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
#
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.57])
AC_INIT([PMlib], [2.1.2], [keno@riken.jp],[PMlib])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR([src/PerfMonitor.cpp])
AC_CONFIG_HEADERS([config.h])


# revision No.
PM_REVISION="20140516_1500"
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
PM_CC="$CC"
AC_SUBST(PM_CC)

case "$PM_CXX" in
  mpicxx)   enable_mpi="wrappermpi" ;;
  mpicc)    enable_mpi="wrappermpi" ;;
  mpiCC)    enable_mpi="wrappermpi" ;;
  mpic++)   enable_mpi="wrappermpi" ;;
  mpiFCCpx) enable_mpi="wrappermpi" ;;
  mpiFCC)   enable_mpi="wrappermpi" ;;
  CC)       enable_mpi="wrappermpi" ;;
esac


AC_SUBST(MPI_DIR)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_LDFLAGS)
AC_SUBST(MPI_LIBS)


#
# if wrapper compiler is specified, skip this part
#

if test x"$enable_mpi" = x"none" ; then
  CXXFLAGS=-D_PM_WITHOUT_MPI_
  CFLAGS=-D_PM_WITHOUT_MPI_
else

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
# PAPI environment
#
enable_papi="none"

AC_SUBST(PAPI_DIR)

AC_ARG_WITH( papi, [AC_HELP_STRING([--with-papi=dir],[Specify PAPI install directory])], , with_papi=none)
if test x"$with_papi" == x"yes" ; then
  enable_papi="yes"
elif test x"$with_papi" != x"none" ; then
  PAPI_DIR=$with_papi;
  enable_papi="yes"
fi


#
# example
#
AC_ARG_WITH(example, [AC_HELP_STRING([--with-example=(no|yes)],[make example [no]])], , with_example=no)



#
# PM special flags
#
PM_CFLAGS="-I$PM_INST_DIR/include"
AC_SUBST(PM_CFLAGS)


if test x"$enable_papi" == x"yes" ; then
  PM_CFLAGS="$PM_CFLAGS"" -DUSE_PAPI"
  CXXFLAGS="$CXXFLAGS"" -DUSE_PAPI"
  CFLAGS="$CFLAGS"" -DUSE_PAPI"
fi


PM_LDFLAGS="-L$PM_INST_DIR/lib"
AC_SUBST(PM_LDFLAGS)

PM_LIBS="-lPM"
AC_SUBST(PM_LIBS)



#
# MAKE_SUB_DIRS
#
AC_SUBST(MAKE_SUB_DIRS)
MAKE_SUB_DIRS="src doc"

if test x"$enable_papi" == x"yes" ; then
  MAKE_SUB_DIRS="$MAKE_SUB_DIRS"" src_papi_ext"
fi

if test x"$with_example" = x"yes" ; then
  MAKE_SUB_DIRS="$MAKE_SUB_DIRS"" example"
fi


#
# Checks for programs.
#
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_CC_C_O


# Checks for libraries.

#
# Checks for header files.
#
AC_CHECK_HEADERS([sys/time.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T


# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday sqrt])

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 src_papi_ext/Makefile \
                 example/Makefile \
                 doc/Makefile \
                 pm-config \
                 include/pmVersion.h \
		])
AC_OUTPUT

chmod +x ./pm-config

