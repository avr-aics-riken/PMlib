#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <PerfMonitor.h>
#include <string>
using namespace pm_lib;
//	char parallel_mode[] = "Serial";
char parallel_mode[] = "Hybrid";
//	char parallel_mode[] = "OpenMP";
//	char parallel_mode[] = "FlatMPI";

#define MATSIZE 1000
int nsize;
struct {
	int nsize;
	float a2[MATSIZE][MATSIZE];
	float b2[MATSIZE][MATSIZE];
	float c2[MATSIZE][MATSIZE];
} matrix;
extern "C" void set_array(), subkerel();
int my_id, npes, num_threads;

PerfMonitor PM;

int main (int argc, char *argv[])
{
	double flop_count;
	double t1, t2;
	int i, j, loop, num_threads;
	enum timing_key {
		tm_sec1,
		tm_sec2,
		tm_END_,
	};

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	char* c_env = std::getenv("OMP_NUM_THREADS");
	if (c_env == NULL) {
		fprintf (stderr, "\n\t OMP_NUM_THREADS is not defined. It will be set as 1.\n");
		omp_set_num_threads(1);
	}
	num_threads  = omp_get_max_threads();
	nsize=MATSIZE;
	matrix.nsize = nsize;
	if(my_id == 0) {
		fprintf(stderr, "<main> MATSIZE=%d max_threads=%d\n", MATSIZE, num_threads);
	}

	PM.setRankInfo(my_id);
	PM.initialize(tm_END_);
	PM.setProperties(tm_sec1, "section1", PerfMonitor::CALC);
//	PM.setProperties(tm_sec1, "section1", PerfMonitor::COMM);
//	PM.setProperties(tm_sec1, "section1", PerfMonitor::CALC, false);
	num_threads  = omp_get_max_threads();
	PM.setParallelMode(parallel_mode, num_threads, npes);

	fprintf(stderr, "\n<main> starting the measurement\n");
	set_array();

	loop=3;
	PM.start(tm_sec1);
	for (i=1; i<=loop; i++){
		t1=MPI_Wtime();
		subkerel();
		t2=MPI_Wtime();
		MPI_Barrier(MPI_COMM_WORLD);
		if(my_id == 0) { fprintf(stderr, "<main> step %d finished in %f seconds\n", i,t2-t1);}
	}
	PM.stop (tm_sec1);

	PM.setProperties(tm_sec2, "section2", PerfMonitor::CALC);
	PM.start(tm_sec2);
	subkerel();
	flop_count=pow ((double)nsize, 3.0)*4.0;
	PM.stop (tm_sec2, flop_count, 1);
	if(my_id == 0) {
		fprintf(stderr, "<section2> flop count in the source code: %15.0f\n\n", flop_count);
	}

	PM.gather();
	PM.print(stdout, "", "Mr. Bean");
	PM.printDetail(stdout);

	MPI_Finalize();
	return 0;
}

