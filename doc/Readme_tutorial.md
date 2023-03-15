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
	test2/	fortran MPI program, and some variation programs
	test3/	C MPI program
	test4/	MPI program managing multiple MPI groups
	test5/	MPI program with split communicators.

src_tutorial/

	example program with coase grain parallel thread structure.


## Brief explanation

Below is a brief note showing how to run the the user application calling PMlib,
and to produce the PMlib report from the job.
See the material in the turorial/ subdirectory for extended explanation
regarding installation/running/reporting.

### Running PMlib instrumented applications and obtaining the performance report

See example shell scripts under scripts/\*/

### useful environment variables

The following environment variables can be used for controlling the PMlib report contents.

#### PMLIB_REPORT

Set the level details of PMlib consumption report.
Choose the value from one of [BASIC, DETAIL, FULL].

	PMLIB_REPORT=BASIC (default)
		Produce basic report. the average performance statistics and power consumption statistics, if any.
	PMLIB_REPORT=DETAIL
		Produce detail report. the statistics for all the processes.
	PMLIB_REPORT=FULL
		Produce detai report as well as the thread performance statistics for each process.

Several example PMlib reports are shown in the log file log_reports/RUN-MPI.1705155.out .

#### HWPC_CHOOSER

Set the type of the hardware performance counter event groups to report.
Choose the value from one of [FLOPS, BANDWIDTH, VECTOR, CACHE, CYCLE, LOADSTORE, USER].

	HWPC_CHOOSER=FLOPS (default)
		floating point operations for single precision and for double precision,
		and their related performance in flops.
	HWPC_CHOOSER=BANDWIDTH
		memory read write counts, and their related performance in bandwidth.
	HWPC_CHOOSER=VECTOR
		floating point operations by vector instructions and scalar operations,
		and their related percentage.
	HWPC_CHOOSER=CACHE
		memory load and store instructions, cache hits and misses,
		and their related percentage.
	HWPC_CHOOSER=LOADSTORE
		memory load and store instructions per vector/SIMD/scalar instructions,
		gather/scatter instructions.
	HWPC_CHOOSER=CYCLE
		total cycles and instructions
	HWPC_CHOOSER=USER
		User provided argument values, aka Arithmetic Workload,
		are accumulated and reported.

#### POWER_CHOOSER

Controlls the contents of the power consumption report.

	POWER_CHOOSER=OFF (default)
		do not report the power consumption stats.
	POWER_CHOOSER=NODE
		report the node stats as the group of combined CMGs, MEMORY, Tofu+AC.
	POWER_CHOOSER=NUMA
		report the stats grouped per numa node.
	POWER_CHOOSER=PARTS
		report the breakdown of all the parts.

#### BYPASS_PMLIB

Set any value to BYPASS_PMLIB to skip all PMlib statistics and report procedures.
PMlib is always enabled unless this BYPASS_PMLIB is set.

#### OTF_TRACING

Set ON for producing the time history file in OTF format.
default value NO.

