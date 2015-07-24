#ifdef _PM_WITHOUT_MPI_
#else
#include <mpi.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <math.h>
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
void set_array();
void subkerel();
double my_timer_(void);
int my_id, npes, num_threads;
// parallel_mode[] should be "Serial", "OpenMP", "FlatMPI", or "Hybrid"

PerfMonitor PM;

int main (int argc, char *argv[])
{
	double flop_count, bandwidth_count, dsize;
	double t1, t2;
	int i, j, loop, num_threads;

#ifdef _PM_WITHOUT_MPI_
	my_id=0;
	npes=1;
#else
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);
#endif


#ifdef _PM_WITHOUT_MPI_
	#ifdef _OPENMP
   	char parallel_mode[] = "OpenMP";
	#else
   	char parallel_mode[] = "Serial";
	#endif
#else
	#ifdef _OPENMP
   	char parallel_mode[] = "Hybrid";
	#else
   	char parallel_mode[] = "FlatMPI";
	#endif
#endif
#ifdef _OPENMP
	char* c_env = std::getenv("OMP_NUM_THREADS");
	if (c_env == NULL) {
		omp_set_num_threads(1);	// OMP_NUM_THREADS was not defined. set as 1
	}
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif

	nsize=MATSIZE;
	matrix.nsize = nsize;
	dsize = (double)nsize;
	if(my_id == 0) {
		fprintf(stderr, "\t<main> MATSIZE=%d max_threads=%d\n", MATSIZE, num_threads);
	}
	fprintf(stderr, "\t<main> starting process %d/%d\n", my_id, npes);

	PM.initialize();
	PM.setRankInfo(my_id);

	PM.setParallelMode(parallel_mode, num_threads, npes);

	PM.setProperties("First location", PerfMonitor::CALC);
	PM.setProperties("Second location", PerfMonitor::CALC);
	PM.setProperties("Third location", PerfMonitor::COMM);

	if(my_id == 0) fprintf(stderr, "\t<main> starting the measurement\n");

	set_array();

	loop=3;
	for (i=1; i<=loop; i++){
	PM.start("First location");
		t1=my_timer_();
		subkerel();
		t2=my_timer_();
		if(my_id == 0) { fprintf(stderr, "<main> step %d finished in %f seconds\n", i,t2-t1);}
	PM.stop ("First location");
	}

	PM.start("Second location");
	subkerel();
	flop_count=pow (dsize, 3.0)*4.0;
	PM.stop ("Second location", flop_count, 1);
	if(my_id == 0) {
		fprintf(stderr, "<main> Second - flop count in the source code: %15.0f\n\n", flop_count);
	}

	PM.start("Third location");
	subkerel();
	bandwidth_count=pow (dsize, 3.0)*2.0 + dsize*dsize*2.0;
	PM.stop ("Third location", bandwidth_count, 1);


	if(my_id == 0) {
		fprintf(stderr, "<main> Third - count in the source code: %15.0f\n\n", bandwidth_count);
	}

	PM.gather();
	PM.print(stdout, "", "Mr. Bean");
	PM.printDetail(stdout);

#ifndef _PM_WITHOUT_MPI_
	MPI_Finalize();
#endif

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



void set_array()
{
int i, j, nsize;

nsize = matrix.nsize;

#pragma omp parallel
#pragma omp for
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
	matrix.a2[i][j] = sin((float)j/(float)nsize);
	matrix.b2[i][j] = cos((float)j/(float)nsize);
	matrix.c2[i][j] = 0.0;
	}
	}
}

void subkerel()
{
int i, j, k, nsize;
float c1,c2,c3;
nsize = matrix.nsize;
#pragma omp parallel
#pragma omp for
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
		c1=0.0;
		for (k=0; k<nsize; k++){
		c2=matrix.a2[i][k] * matrix.a2[j][k];
		c3=matrix.b2[i][k] * matrix.b2[j][k];
		c1=c1 + c2+c3;
		}
		matrix.c2[i][j] = matrix.c2[i][j] + c1/(float)nsize;
	}
	}
}

