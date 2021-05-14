# PMlib tutorials

## tutorial material

The following subdirectories contain the PMlib tutorial material
and typical example source programs calling PMlib, as well as the
compiling and execution job scripts for popular systems.

tutorial/

Detail tutorial of installing PMlib and using PMlib from applications.

	+ PMlib-Getting-Started.pdf: slides (Nihongo version only.)

scripts/

Each subdirectory contains the example shell scripts for installing PMlib
and for running PMlib applications on the named platform.

	Fugaku/    	supercomputer Fugaku
	GNU/       	Intel X86 PC with GNU compilers
	Intel/     	Intel Xeon servers(Skylake/Ivybridge/SNB) with Intel compilers
	fx100/     	Fujitsu Sparc XIfx based FX100
	fx700/		Fujitsu A64FX based HPC system 

../example/

	The example application source programs calling PMlib. All the programs call
	MPI routines. Some examples can be single process using -DDISABLE_MPI macro.
	The example applications can be built for testing as a part of the
	installation. See Readme.md in the top directory.

	test1/	C++ MPI program
	test2/	C++ program with user C routines
	test4/	fortran MPI program
	test3/	MPI program managing multiple MPI groups
	test5/	MPI program with split communicators.

src_tutorial/

	additional example program with coase grain parallel thread structure.


## Brief explanation

Below is a brief note showing how to run the the user application calling PMlib,
and to produce the PMlib report from the job.
See the material in the turorial/ subdirectory for extended explanation
regarding installation/running/reporting.

### Running PMlib instrumented applications

### obtaining the performance report

### useful environment variables

- HWPC_CHOOSER

HWPC_CHOOSER is used for setting the type of the hardware performance counter
event groups shown in the PMlib report.
Set the value of HWPC_CHOOSER from one of the following texts:
{FLOPS, BANDWIDTH, VECTOR, CACHE, CYCLE, LOADSTORE, USER}

	- HWPC_CHOOSER=FLOPS:
		floating point operations for single precision and for double precision,
		and their related performance in flops.
	- HWPC_CHOOSER=BANDWIDTH:
		memory read write counts, and their related performance in bandwidth.
	- HWPC_CHOOSER=VECTOR:
		floating point operations by vector instructions and scalar operations,
		and their related percentage.
	- HWPC_CHOOSER=CACHE:
		memory load and store instructions, cache hits and misses,
		and their related percentage.
	- HWPC_CHOOSER=LOADSTORE:
		memory load and store instructions per vector/SIMD/scalar instructions,
		gather/scatter instructions.
	- HWPC_CHOOSER=CYCLE:
		total cycles and instructions
	- HWPC_CHOOSER=USER:
		User provided argument values, aka Arithmetic Workload,
		are accumulated and reported.


- PMLIB_REPORT

PMLIB_REPORT is used for setting the level details of PMlib
statisticcs report.
Set the value of PMLIB_REPORT from one of the followings:
{BASIC, DETAIL, ALLTHREADS}.
The example PMlib statistics reports by an application job with
PMLIB_REPORT=3 setting
for each of the HWPC_CHOOSER choice are shown in the log file
log_reports/RUN-MPI.1705155.out .



- BYPASS_PMLIB

BYPASS_PMLIB disables PMlib statistics counting and reporting.

- OTF_TRACING

OTF_TRACING can be used for producing the time history file in OTF format.


