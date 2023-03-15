# PMlib - Performance Monitor library

* Copyright (c) 2010-2011 VCAD System Research Program, RIKEN. All rights reserved.
* Copyright (c) 2012-2023 RIKEN Center for Computational Science (R-CCS). All rights reserved.
* Copyright (c) 2016-2023 Research Institute for Information Technology (RIIT), Kyushu University. All rights reserved.

## OUTLINE

This library records the statistics information of run-time performance and the trace information of a user code and reports its summary. The PMlib is able to use for both serial and parallel environments including hybrid(OpenMP & MPI) code. In addition, PAPI interface allows us to access the information of build-in hardware counter.

## SOFTWARE REQUIREMENT
- Compilers for Fortran/C/C++
- Cmake
- MPI library  (optional)
- PAPI library (optional)
- Power API library (optional)
- OTF library (optional)

Although MPI and PAPI libraries are optional, they are frequently used in the application, and are recommended to be included.


## INGREDIENTS
~~~
ChangeLog         History of development
License.txt       License to apply
Makefile_hand     Makefile for hand compile
Readme.md         This document, including the description of build
cmake/            Modules of cmake
doc/              Document
  Doxyfile        Configuration file to generate a doxygen file
  scripts/        Shell script collections for installation/execution
  src_advanced/   Advanced example programs calling PMlib APIs
  src_tutorial/   Example tutorial programs for beginners
  tutorial/       Tutorial documents
example/          Example source codes
include/          Header files
src/              Source codes
src_papi_ext/     Extension of PAPI interface
src_power_ext/    Extension of Power API interface
src_otf_ext/      Extension of Open Tracer Format interface
~~~

## HOW TO BUILD

Typical installation will be composed of three steps.

1. Confirm that the software requirement is met.
2. Obtain the the package tar ball and unpack it under the temporary directory.
3. Configure and Make the related files.


### max_nthreads

Maximum number of the measuable threads per process can be configured. The default is 48.
Change the value of `const int Max_nthreads` in `~/include/pmlib_papi.h`.


### Build

~~~
$ export PM_HOME=/hogehoge
$ mkdir BUILD
$ cd BUILD
$ cmake [options] ..
$ make
$ sudo make install
~~~


### Options

`-D INSTALL_DIR=install_directory`

>  Specify the directory where PMlib will be installed. PMlib libraries are installed at `install_directory/lib` and the header files are placed at `install_directory/include`. The default install directory is `/usr/local/PMlib`.

`-D with_example=` {no | yes}

>  This option turns on compiling sample codes. Default is no.

`-D enable_Fortran=` {no | yes}

> This option tells a compiler to use a Fortran.

`-D with_MPI=` {no | yes}

>  If you use an MPI library, specify `with_MPI=yes`.

`-D enable_OPENMP=` {no | yes}

> Enable OpenMP directives.  <!-- This option is valid if only PAPI interface is enabled. ??? -->

`-D with_PAPI=` {no | yes | installed_directory}

>  If you use PAPI library, specify this option with PAPI_DIR value pointing to the PAPI library installed directory. In cross-compile installation, there can be multiple PAPI libraries on the system, one for the current platform and another for the target platform. See examples in 4. INSTALLATION EXAMPLES section. In many cases, `-Dwith_PAPI="yes"` can detect the correct path.

`-D with_POWER=` {no | yes | installed_directory}

>  This option is exclusively available on supercomputer Fugaku and on FX1000 systems. Power API developed by Sandia National Laboratory is integrated into PMlib.  Specifying `-Dwith_POWER="yes"` should detect the correct path.

`-D with_OTF=` {no | installed_directory}

>  If you use OTF library, specify this option `with_OTF` value pointing to the OTF library installed directory.

`-D enable_PreciseTimer=` {yes | no}

> Precise timers are available on some platforms. This option provides -DUSE_PRECISE_TIMER to C++ compiler option CMAKE_CXX_FLAGS when building the PMlib library.
~~~
-DUSE_PRECISE_TIMER            # for Intel Xeon
-DUSE_PRECISE_TIMER -Nfjcex    # for K computer and FX100 and TCS env.?
~~~

### Default options
~~~
with_example = OFF
enable_Fortran = OFF
with_MPI = OFF
enable_OPENMP = OFF
with_PAPI = OFF
with_POWER = OFF
with_OTF = OFF
enable_PreciseTimer = ON
~~~

The default compiler options are described in `cmake/CompilerOptionSelector.cmake` file. See BUILD OPTION section in CMakeLists.txt in detail.


## Cmake Examples

### INTEL/GNU/PGI compiler

####
##### serial version
In this document, serial version means single process (non-MPI) version
with possible OpenMP thread parallel model, if OpenMP is available on the
target platform.
On most system, CC/CXX/F90/FC and other environment variables must be set
for choosing the right compilers.
CMAKE_*_FLAGS options can be used for passing compiler options as shown below.
Adding OpenMP options and fast timer option (-DUSE_PRECISE_TIMER) is generally
recommended
Note that C++11 or later standard must be supported for CXXFLAGS.
on Intel Xeon platform.

~~~
Intel: CC=icc  CXX=icpc F90=ifort FC=ifort
		CXXFLAGS="-qopenmp -DUSE_PRECISE_TIMER -std=c++11" FCFLAGS="-qopenmp -fpp "
PGI  : CC=pgcc CXX=pgc++ F90=pgf90 FC=pgfortran
		CXXFLAGS="-mp -std=c++11" FCFLAGS="-mp -Mpreprocess" LDFLAGS="-pgc++libs"
GNU  : CC=gcc  CXX=g++  F90=gfortran FC=gfortran
		CXXFLAGS="-fopenmp -std=c++11" FCFLAGS="-fopenmp -cpp"
~~~

An example of cmake command and options for Intel compiler is shown below.

~~~
$ # Intel compiler SERIAL
$ export CC=icc CXX=icpc F90=ifort FC=ifort
$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
	-Denable_OPENMP=yes \
	-Dwith_MPI=no \
	-Denable_Fortran=yes \
	-Dwith_example=no \
	-Dwith_PAPI=no \
	-Dwith_OTF=no \
  -Denable_PreciseTimer=yes ..
~~~

##### MPI version
In this document, MPI version means multi process (MPI) version with possible
OpenMP thread parallel model, if OpenMP is available on the target platform.
If PAPI and/or OTF library is available on the system, set their path
as the example below to activate the functionality within PMlib.

~~~
$ # Intel compiler example
$ export CC=mpiicc CXX=mpiicpc F90=mpiifort FC=mpiifort
$ PAPI_DIR="#set PAPI library path such as /usr/local/papi-5.5"
$ OTF_DIR="#set OTF library path such as /usr/local/otf-1.12"
$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
	-Denable_OPENMP=yes \
	-Dwith_MPI=yes \
	-Denable_Fortran=yes \
	-Dwith_example=yes \
	-Dwith_PAPI="${PAPI_DIR}" \
	-Dwith_OTF="${OTF_DIR}" \
  -Denable_PreciseTimer=yes ..
~~~


### Fugaku supercomputer with traditional compilers (i.e. not clang mode compilers)

~~~
$ PAPI_DIR=/opt/FJSVxos/devkit/aarch64/rfs/usr
$ CXXFLAGS="-Kocl "
$ CFLAGS="-Kocl  "
$ FFLAGS="-Kocl  "
$ cmake \
		-DCMAKE_CXX_FLAGS="${CXXFLAGS} " \
		-DCMAKE_C_FLAGS="${CFLAGS} " \
		-DCMAKE_Fortran_FLAGS="${FFLAGS} " \
		-DINSTALL_DIR=${PM_HOME}/PMlib \
		-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fugaku.cmake \
		-Denable_OPENMP=yes \
		-Dwith_MPI=yes \
		-Denable_Fortran=yes \
		-Dwith_example=yes \
		-Dwith_PAPI="${PAPI_DIR}" \
		-Dwith_OTF=no \
		..

~~~


### FUJITSU FX100, K computer (Cross compilation on login nodes) and Fujitsu TCS environment for intel PC

~~~
$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
            -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fx10.cmake \
            -Denable_OPENMP=no \
            -Dwith_MPI=yes \
            -Denable_Fortran=no \
            -Dwith_example=no \
            -Dwith_PAPI=no \
            -Dwith_OTF=no \
            -Denable_PreciseTimer=yes ..

$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
            -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fx100.cmake \
            -Denable_OPENMP=no \
            -Dwith_MPI=yes \
            -Denable_Fortran=no \
            -Dwith_example=no \
            -Dwith_PAPI=no \
            -Dwith_OTF=no \
            -Denable_PreciseTimer=yes ..

$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
            -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_K.cmake \
            -Denable_OPENMP=no \
            -Dwith_MPI=yes \
            -Denable_Fortran=no \
            -Dwith_example=no \
            -Dwith_PAPI=no \
            -Dwith_OTF=no \
            -Denable_PreciseTimer=yes ..

$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
            -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_ITO_TCS.cmake \
            -Denable_OPENMP=no \
            -Dwith_MPI=yes \
            -Denable_Fortran=no \
            -Dwith_example=no \
            -Dwith_PAPI=no \
            -Dwith_OTF=no ..
~~~
            
#### F_TCS environment serial

~~~
$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_F_TCS.cmake \
-Denable_OPENMP=no \
-Dwith_MPI=no \
-Denable_Fortran=no \
-Dwith_example=no \
-Dwith_PAPI=no \
-Dwith_OTF=no \
-Denable_PreciseTimer=yes ..
~~~


##### Note
- On Fujitsu machines(fx10, K, fx100), confirm appropriate directory path for compiler environment.
- Before rebuilding, execute the following command for cleaning up.

~~~
$ make distclean
~~~


### Other platforms

Refer to `Makefile_hand` hand for manually prepare the Makefiles for
a system not covered in the previous sections.
Edit environmental variables in a `Makefile_hand` file for a target machine.


## RUNNING THE APPLICATION WITH PMLIB

There are several documents explaining how to run the application with PMlib, and how to understand the output information.
Currently, they are all written in Japanese.

Readme.md      : This document
doc/Readme.md  : How to generate the detail API specification with Doxygen
doc/tutorial/PMlib-Getting-Started.pdf      : Quick start guide
doc/PMlib.pdf                               : detail user guide (outdated)


## RUN TIME ENVIRONMENT VARIABLES

When running applications linked with PMlib, the following
environment variables can be set to the shell.

`PMLIB_REPORT=(BASIC|DETAIL|FULL)`

This environment variable controlls the level of details in the PMlib performance statistics report.
The value BASIC will provide the summary report of the averaged values of the all processes.
The value DETAIL will provide the statistics report for all the processes.
The value FULL will provide the statistics report for all the threads of all the processes.
Note that the amount of the report is decided by the number of processes, the number of threads, the choice of HWPC_CHOOSER.

`HWPC_CHOOSER=(FLOPS|BANDWIDTH|VECTOR|LOADSTORE|CACHE|CYCLE)`

If this environment variable is set, PMlib automatically detects the PAPI based hardware counters. If this environment variable is not set, the HWPC counters are not reported.

`OTF_TRACING=(off|on|full)`

If this environment variable is set, PMlib automatically generates the Open Trace Format files for post processing. There will be three type of OTF files.

~~~
  ${OTF_FILENAME}.otf
  ${OTF_FILENAME}.0.def
  ${OTF_FILENAME}.(1|2|..|N).events
~~~

See the next environment variable `OTF_FILENAME`. If the value is "off" or not defined, the OTF files are not produced. If the value is "on", the OTF files will contain the timer information only. If the value is "full", the OTF files will contain the counter information as well as the timer information. Remark that `OTF_TRACING=full` may yield the heavy overhead, if the measuring sections are repeated many times.

`OTF_FILENAME="some file name"`

The value of `${OTF_FILENAME}` is used to prefix the OTF file names if the value of previous `${OTF_TRACING}` has been set to "on" or "full". If this environment variable is not set, the default value of `"pmlib_optional_otf_files"` is used to prefix the OTF file names.


## EXAMPLES

* If you specify the test option by `-Denable_example=yes`, you can
execute the intrinsic tests by;

	`$ make test` or `$ ctest`

* The detailed results are written in `BUILD/Testing/Temporary/LastTest.log` file.
Meanwhile, the summary is displayed for stdout.


## PAPI install (intel compiler)

~~~
$ ./configure --prefix=${PREFIX} CC=icc F77=ifort
$ make
$ make install-all
~~~


## CONTRIBUTORS

* Kenji     Ono      *keno@{cc.kyushu-u.ac, riken}.jp*
* Kazunori  Mikami   kazunori.mikami@riken.jp
* Soichiro  Suzuki
* Syoyo     Fujita
