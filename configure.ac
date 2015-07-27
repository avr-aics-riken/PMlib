##############################################################################
#
# PMlib - Performance Monitor library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2015 Advanced Institute for Computational Science, RIKEN.
# All rights reserved.
#
##############################################################################
#
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.57])

AC_INIT([PMlib], [4.0.1], [keno@riken.jp],[PMlib])

echo "----------------------------------------"
echo "Running PMlib Configure Script"
echo "----------------------------------------"


AC_CONFIG_SRCDIR([src/PerfMonitor.cpp])


#
# Specify directory for auxillary build tools (e.g., install-sh, config.sub, config.guess) and M4 files.
#

AC_CONFIG_AUX_DIR(config)


#
# To prevent the source codes from breaking, we suppress to do configure in the source directory.
#

if test -e configure.ac ; then
echo "Please run configure from a separate build directory. It is not allowd to run configure in the source directory."
echo "Pleaes try ..."
echo " "
echo "$ cd BUILD_DIR"
echo "$ ../configure [options]"
exit
fi


#
# Configure should create config.h from config.h.in
#

AC_CONFIG_HEADERS([config.h])


#
# Use automake
#

AM_INIT_AUTOMAKE()


#
# revision No.
#

PM_REVISION="20150727_1300"
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
MPI_EXT=""


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
  mpiicc)   enable_mpi="wrappermpi" ;;
  mpiicpc)  enable_mpi="wrappermpi" ;;
esac


AC_SUBST(MPI_DIR)
AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPI_LDFLAGS)
AC_SUBST(MPI_LIBS)


#
# if wrapper compiler is specified, skip this part
#

if test x"$enable_mpi" = x"none" ; then
  CXXFLAGS="$CXXFLAGS -D_PM_WITHOUT_MPI_"
  CFLAGS="$CFLAGS -D_PM_WITHOUT_MPI_"
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
  MPI_EXT="_MPI"
fi
AC_SUBST(MPI_EXT)


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

# PAPI_TARGET = true, if use papi
AM_CONDITIONAL([PAPI_TARGET], [test "$enable_papi" = "yes"])


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
  PM_LIBS="-lPM$MPI_EXT -L$PAPI_DIR/lib64 -lpapi -lpfm -L../src_papi_ext -lpapi_ext"
else
  PM_LIBS="-lPM$MPI_EXT"
fi

AC_SUBST(PM_LIBS)

PM_LDFLAGS="-L$PM_INST_DIR/lib"
AC_SUBST(PM_LDFLAGS)





#
# MAKE_SUB_DIRS
#
AC_SUBST(MAKE_SUB_DIRS)
MAKE_SUB_DIRS="src doc"

if test x"$enable_papi" == x"yes" ; then
MAKE_SUB_DIRS="$MAKE_SUB_DIRS"" src_papi_ext"
fi

if test x"$with_example" = x"yes" ; then
MAKE_SUB_DIRS="$MAKE_SUB_DIRS"" example/test1"
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
AC_CHECK_FUNCS([gethostname gettimeofday sqrt])

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 src_papi_ext/Makefile \
                 example/test1/Makefile \
                 doc/Makefile \
                 pm-config \
                 include/pmVersion.h \
                ])
AC_OUTPUT

chmod +x ./pm-config

echo "---------------------------------------------"
echo "Finished Running PMlib Configure Script"
echo "---------------------------------------------"
