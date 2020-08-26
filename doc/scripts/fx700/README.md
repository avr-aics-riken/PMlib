As of 2020/8/25, the PAPI library provided by Fujitsu does not work on FX700.
We are waiting for Fujitsu to provide the correct PAPI library built for
the system.
Thus, this directory includes the installation scripts only.

	x.cmake-fx700.sh : installation script
	stdout.make-fx700 : stdout from above script

i.e., the following run scripts do not work until the correct library
is installed.

	x.run-mpi.sh
	x.run-single.sh
