#include <mpi.h>
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
struct matrix {
	int nsize;
	float a2[MATSIZE][MATSIZE];
	float b2[MATSIZE][MATSIZE];
	float c2[MATSIZE][MATSIZE];
} matrix;
void set_array();
void somekernel();
void slowkernel();
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
		fprintf(stderr, "\t<main> starts. npes=%d, MATSIZE=%d max_threads=%d\n",
			npes, MATSIZE, num_threads);
	}
	fprintf(stderr, "\t\tstarting process:%d\n", my_id);

	PM.initialize();

	PM.setProperties("First section", PerfMonitor::CALC);
	PM.setProperties("Second section", PerfMonitor::CALC, false);
	PM.setProperties("Subsection X", PerfMonitor::COMM);
	PM.setProperties("Subsection Y", PerfMonitor::CALC);

	set_array();

// checking exclusive section
	PM.start("First section");
		somekernel();
	flop_count=pow (dsize, 3.0)*4.0;
	PM.stop ("First section", flop_count, 1);
	if(my_id == 0) {
		fprintf(stderr, "\t<First section> flop_count=%15.0f\n", flop_count);
	}
	spacer();

// checking non-exclusive section
	PM.start("Second section");
	spacer();

	for (i=0; i<3; i++){
		PM.start("Subsection X");
		slowkernel();
		byte_count=pow (dsize, 3.0)*4.0*4.0;
	
		PM.stop ("Subsection X", byte_count, 1);
		spacer();
		if(my_id == 0) {
			fprintf(stderr, "\t<Subsection X> byte_count=%15.0f\n", byte_count);
		}

		PM.start("Subsection Y");
		somekernel();
		flop_count=pow (dsize, 3.0)*4.0;
		PM.stop ("Subsection Y", flop_count, 1);
		spacer();
		if(my_id == 0) {
			fprintf(stderr, "\t<Subsection Y> flop_count=%15.0f\n", flop_count);
		}
	}


	somekernel();
	flop_count=pow (dsize, 3.0)*4.0 * (2*3+1) ;
	PM.stop("Second section", flop_count, 1);
	spacer();

	if(my_id == 0) fprintf(stderr, "\t<main> starting PM.gather().\n");
	PM.gather();
	if(my_id == 0) fprintf(stderr, "\t<main> starting PM.print().\n");
	PM.print(stdout, "", "Mrs. Kobe", 0);
	PM.printDetail(stdout, 1);

	MPI_Finalize();
	return 0;
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

// some computing kernel
void somekernel()
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

// slow computing kernel
void slowkernel()
{
int i, j, k, nsize;
float c1,c2,c3;
nsize = matrix.nsize;
#pragma omp parallel
#pragma omp for
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
		c1=0.0;
#pragma novector
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
