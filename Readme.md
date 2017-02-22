# PMlib - Performance Monitor library

* Copyright (c) 2010-2011 VCAD System Research Program, RIKEN. All rights reserved.
* Copyright (c) 2012-2017 Advanced Institute for Computational Science (AICS), RIKEN. All rights reserved.
* Copyright (c) 2016-2017 Research Institute for Information Technology (RIIT), Kyushu University. All rights reserved.

## OUTLINE

This library records the statistics information of run-time performance and the trace information of a user code and reports its summary. The PMlib is able to use for both serial and parallel environments including hybrid(OpenMP & MPI) code. In addition, PAPI interface allows us to access the information of build-in hardware counter.

## SOFTWARE REQUIREMENT
- Cmake
- MPI library  (option)
- PAPI library (option)
- OTF library (option)

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
  PMlib.pdf       User's manual (in Japanese)
  scripts/        Shell script collections for installation/execution
  tutorial/       Tutorial documents
example/          Example source codes
include/          Header files
src/              Source codes
src_papi_ext/     Extension of PAPI interface
src_otf_ext/      Extension of Open Tracer Format interface
~~~

## HOW TO BUILD

Typical installation will be composed of three steps.

1. Confirm that the software requirement is met.
2. Obtain the the package tar ball and unpack it under the temporary directory.
3. Configure and Make the related files.

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

>  Specify the directory that this library will be installed. Built library is installed at `install_directory/lib` and the header files are placed at `install_directory/include`. The default install directory is `/usr/local/PMlib`.

`-D with_example=` {no | yes}

>  This option turns on compiling sample codes. Default is no.

`-D enable_Fortran=` {no | yes}

> This option tells a compiler to use a Fortran.

`-D with_MPI=` {no | yes}

>  If you use an MPI library, specify `with_MPI=yes`.

`-D enable_OPENMP=` {no | yes}

> Enable OpenMP directives. This option is valid if only PAPI interface is enabled.

`-D with_PAPI=` {no | yes | installed_directory}

>  If you use PAPI library, specify this option with PAPI_DIR value pointing to the PAPI library installed directory. In cross-compile installation, there can be multiple PAPI libraries on the system, one for the current platform and another for the target platform. See examples in 4. INSTALLATION EXAMPLES section. In many cases, `-Dwith_PAPI="yes"` can detect the correct path.

`-D with_OTF=` {no | installed_directory}

>  If you use OTF library, specify this option `with_OTF` value pointing to the OTF library installed directory.

The default compiler options are described in `cmake/CompilerOptionSelector.cmake` file. See BUILD OPTION section in CMakeLists.txt in detail.


## Configure Examples

### INTEL/GNU compiler

~~~
$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib -Denable_OPENMP=no -Dwith_MPI=no -Denable_Fortran=yes -Dwith_example=no -Dwith_PAPI=no -Dwith_OTF=no ..
~~~


### FUJITSU compiler / FX10 on login nodes (Cross compilation)
~~~
$ cmake -DINSTALL_DIR=${PM_HOME}/PMlib \
            -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_fx10.cmake \
            -Denable_OPENMP=no \
            -Dwith_MPI=yes \
            -Denable_Fortran=no \
            -Dwith_example=no \
            -Dwith_PAPI=no \
            -Dwith_OTF=no ..
~~~


### FUJITSU compiler / K computer on login nodes (Cross compilation)

~~~
$ cmake -DINSTALL_DIR=${PMT_HOME}/PMlib \
            -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_K.cmake \
            -Denable_OPENMP=no \
            -Dwith_MPI=yes \
            -Denable_Fortran=no \
            -Dwith_example=no \
            -Dwith_PAPI=no \
            -Dwith_OTF=no ..
~~~


##### Note
- Before building, execute following command for clean. `$ make distclean`


### Other platforms

Refer to `Makefile_hand` hand for manually prepare the Makefiles for
a system not covered in the previous sections.
Edit environmental variables in a `Makefile_hand` file for a target machine.


## RUNNING THE APPLICATION WITH PMLIB

There are several documents explaining how to run the application with PMlib, and how to understand the output information.
Currently, they are all written in Japanese.

doc/PMlib.pdf                                 : detail user guide
doc/tutorial/PMlib-Getting-Started.pdf 			  : Quick start guide
doc/tutorial/Tutorial-slide1-overview.pdf		  : PMlib overview
doc/tutorial/Tutorial-slide2-installation.pdf	: PMlib installation


## RUN TIME ENVIRONMENT VARIABLES

When running applications linked with PMlib, the following
environment variables can be set to the shell.

`HWPC_CHOOSER=(FLOPS|BANDWIDTH|VECTOR|CACHE|CYCLE)`

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




## CONTRIBUTORS

* Kenji     Ono      *keno@{cc.kyushu-u.ac, riken}.jp*
* Kazunori  Mikami   kazunori.mikami@riken.jp
* Soichiro  Suzuki
* Syoyo     Fujita
