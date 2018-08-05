//	#include <mpi.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <math.h>
#include <string>
#include <PerfMonitor.h>
using namespace pm_lib;

#define MATSIZE 1000
int nsize;
struct matrix {
	int nsize;
	float a2[MATSIZE][MATSIZE];
	float b2[MATSIZE][MATSIZE];
	float c2[MATSIZE][MATSIZE];
} matrix;
void set_array();
void somekernel();
void spacer();
int my_id, npes, num_threads;

PerfMonitor PM;

int main (int argc, char *argv[])
{
	double flop_count, byte_count, dsize;
	double t1, t2;
	int i, j, num_threads;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

#ifdef _OPENMP
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif

	nsize=MATSIZE;
	matrix.nsize = nsize;
	dsize = (double)nsize;
	if(my_id == 0) {
		fprintf(stderr, "\t<main> starts. npes=%d, MATSIZE=%d max_threads=%d\n",
			npes, MATSIZE, num_threads);
	}
	fprintf(stderr, "\t\tstarting process:%d\n", my_id);

	PM.initialize();

	PM.setProperties("First section", PerfMonitor::CALC);
	PM.setProperties("Second section", PerfMonitor::CALC, false);
	PM.setProperties("Subsection 1", PerfMonitor::COMM);
	PM.setProperties("Subsection 2", PerfMonitor::CALC);
	// Remard that "Second section" is not exclusive, i.e. inclusive


	set_array();

	PM.start("First section");
	somekernel();
	flop_count=pow (dsize, 3.0)*4.0;
	//	flop_count=3000.0;
	PM.stop ("First section", flop_count, 1);
	spacer();

	PM.start("Second section");
	spacer();

	PM.start("Subsection 1");
	somekernel();
	byte_count=pow (dsize, 3.0)*4.0*4.0;
	PM.stop ("Subsection 1", byte_count, 1);
	spacer();

	PM.start("Subsection 2");
	somekernel();
	flop_count=pow (dsize, 3.0)*4.0 * 1.2;	// somewhat inflated number ...
	PM.stop ("Subsection 2", flop_count, 1);
	spacer();

	somekernel();
	flop_count=pow (dsize, 3.0)*4.0 * 1.5;	// somewhat inflated number ...
	PM.stop("Second section", flop_count, 1);
	spacer();

	PM.gather();
	PM.print(stdout, "", "", 1);
	PM.printDetail(stdout, 0, 0);
	for (i=0; i<npes; i++){
		PM.printThreads(stdout, i, 0);
	}
	PM.printLegend(stdout);

	MPI_Finalize();
	return 0;
}


void set_array()
{
int i, j, nsize;
nsize = matrix.nsize;
#pragma omp parallel private(i,j)
#pragma omp for
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
	matrix.a2[i][j] = sin((float)j/(float)nsize);
	matrix.b2[i][j] = cos((float)j/(float)nsize);
	matrix.c2[i][j] = 0.0;
	}
	}
}

// some computing kernel
void somekernel()
{
int i, j, k, nsize;
float c1,c2,c3;
nsize = matrix.nsize;
#pragma omp parallel private(i,j,k, c1,c2,c3)
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

// add some meaningless space, for easier debug with visualization package
void spacer()
{
	for (int i=0; i<10; i++){ set_array(); }
}
