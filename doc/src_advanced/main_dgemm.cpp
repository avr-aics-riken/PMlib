#include <mpi.h>
#include <stdio.h>
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <PerfMonitor.h>
using namespace pm_lib;
//	char parallel_mode[] = "Serial";
char parallel_mode[] = "Hybrid";
//	char parallel_mode[] = "OpenMP";
//	char parallel_mode[] = "FlatMPI";
// C routines need this declaration
//	extern "C" void sub_initialize();
//	extern "C" void sub_dgemm();
// Fortran routines also need this extern "C". note the _(underscore) in the end
extern "C" void sub_initialize_();
extern "C" void sub_dgemm_();

PerfMonitor PM;
int my_id, npes, num_threads;

int main (int argc, char *argv[])
{

double flop_count;
double tt, t1, t2, t3, t4;
int i, j, loop, num_threads, iret;
float real_time, proc_time, mflops;
long long flpops;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);
#ifdef _OPENMP
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif
	if(my_id == 0) fprintf(stderr, "<main> started. %d PEs x %d Threads\n",
		npes, num_threads);

	PM.initialize();
	//	num_threads  = omp_get_max_threads();
	(void)PM.getVersionInfo();

	if(my_id == 0) fprintf(stderr, "starting benchmark\n");
	t1=MPI_Wtime();

	PM.start("sub_initialize");
	sub_initialize_();
	PM.stop("sub_initialize");

	//	for (int i=0; i<1; i++) {	// 10 seconds per call
	for (int i=0; i<30; i++) {
	PM.start("sub_dgemm");
	sub_dgemm_();
	PM.stop("sub_dgemm");
	}

	t2=MPI_Wtime();
	MPI_Barrier(MPI_COMM_WORLD);
	if(my_id == 0) fprintf(stderr, "benchmark finished in %f seconds\n", t2-t1);

	//	PM.print(stdout, "", "", 0);
PM.report(stdout);

MPI_Finalize();
return 0;
}

