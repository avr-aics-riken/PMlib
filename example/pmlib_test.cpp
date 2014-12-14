#ifdef _PM_WITHOUT_MPI_
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <PerfMonitor.h>
#include <string>
using namespace pm_lib;

#define MATSIZE 1000
int nsize;
struct {
	int nsize;
	float a2[MATSIZE][MATSIZE];
	float b2[MATSIZE][MATSIZE];
	float c2[MATSIZE][MATSIZE];
} matrix;
extern "C" void set_array(), subkerel();
double my_timer_(void);
int my_id, npes, num_threads;

PerfMonitor PM;

int main (int argc, char *argv[])
{
	double flop_count, bandwidth_count, dsize;
	double t1, t2;
	int i, j, loop, num_threads;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	char* c_env = std::getenv("OMP_NUM_THREADS");
	if (c_env == NULL) {
		fprintf (stderr, "\n\t OMP_NUM_THREADS is not defined. It will be set as 1.\n");
		omp_set_num_threads(1);
	}
    char parallel_mode[] = "Hybrid"; //	choose from "Serial", "OpenMP", "FlatMPI", "Hybrid"
	num_threads  = omp_get_max_threads();
	nsize=MATSIZE;
	matrix.nsize = nsize;
	dsize = (double)nsize;
	if(my_id == 0) {
		fprintf(stderr, "\t<main> MATSIZE=%d max_threads=%d\n", MATSIZE, num_threads);
	}

	PM.initialize();
	PM.setRankInfo(my_id);

	PM.setParallelMode(parallel_mode, num_threads, npes);

	PM.setProperties("First location", PerfMonitor::CALC);
	PM.setProperties("Second location", PerfMonitor::CALC);
	PM.setProperties("Third location", PerfMonitor::COMM);
	PM.setProperties("Extra location", PerfMonitor::AUTO);

	fprintf(stderr, "\t<main> starting the measurement\n");
	set_array();

	loop=3;
	PM.start("First location");
	for (i=1; i<=loop; i++){
		t1=my_timer_();
		subkerel();
		t2=my_timer_();
		MPI_Barrier(MPI_COMM_WORLD);
		if(my_id == 0) { fprintf(stderr, "<main> step %d finished in %f seconds\n", i,t2-t1);}
	}
	PM.stop ("First location");

	PM.start("Second location");
	subkerel();
	flop_count=pow (dsize, 3.0)*4.0;
	PM.stop ("Second location", flop_count, 1);
	if(my_id == 0) {
		fprintf(stderr, "<Second location> flop count in the source code: %15.0f\n\n", flop_count);
	}

	PM.start("Third location");
	subkerel();
	bandwidth_count=pow (dsize, 3.0)*2.0 + dsize*dsize*2.0;
	PM.stop ("Third location", bandwidth_count, 1);

	PM.start("Extra location");
	subkerel();
	subkerel();
	PM.stop ("Extra location");

	if(my_id == 0) {
		fprintf(stderr, "<Third location> count in the source code: %15.0f\n\n", bandwidth_count);
	}

	PM.gather();
	PM.print(stdout, "", "Mr. Bean");
	PM.printDetail(stdout);

	MPI_Finalize();
	return 0;
}

// timer routine for both MPI and serial model
// remark that the timer resolution of gettimeofday is only millisecond order
#include        <sys/time.h>
double my_timer_()
{
	struct timeval s_val;
	gettimeofday(&s_val,0);
	return ((double) s_val.tv_sec + 0.000001*s_val.tv_usec);
}

