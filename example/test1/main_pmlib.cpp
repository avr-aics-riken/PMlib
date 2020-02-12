#include <PerfMonitor.h>
//	#include <mpi.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <math.h>
#include <string>
#include <sstream>
using namespace pm_lib;

#define MATSIZE 1000
int nsize;
struct matrix {
	int nsize;
	double a2[MATSIZE][MATSIZE];
	double b2[MATSIZE][MATSIZE];
	double c2[MATSIZE][MATSIZE];
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

    std::string comments;
    std::ostringstream ouch; // old C++ notation

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
	//	fprintf(stderr, "\t\tstarting process:%d\n", my_id);

	PM.initialize();

	PM.setProperties("Initial-section", PerfMonitor::CALC);
	PM.setProperties("Loop-section", PerfMonitor::COMM, false);
	PM.setProperties("Kernel-Slow", PerfMonitor::CALC);
	PM.setProperties("Kernel-Fast", PerfMonitor::CALC);

// checking exclusive section
	PM.start("Initial-section");
	set_array();
	flop_count=pow (dsize, 2.0)*2.0;
	byte_count=pow (dsize, 2.0)*3.0*8.0;
	PM.stop ("Initial-section", flop_count, 1);

	spacer();

	somekernel();
	flop_count=pow (dsize, 3.0)*4.0;
	byte_count=pow (dsize, 3.0)*4.0*8.0;


// checking non-exclusive section
	PM.start("Loop-section");
	spacer();

	for (i=0; i<3; i++){
		PM.start("Kernel-Slow");
		slowkernel();
		PM.stop ("Kernel-Slow", flop_count, 1);
		spacer();

		PM.start("Kernel-Fast");
		somekernel();
		PM.stop ("Kernel-Fast", flop_count, 1);
		spacer();

		//check//	comments = "iteration i:" + std::to_string(i);
		//check//	PM.printProgress(stdout, comments, 1);
	}

	//check//	PM.postTrace();

	somekernel();
	PM.stop("Loop-section", byte_count*(2*3), 1);
	spacer();

	PM.print(stdout, "", "Mrs. Kobe", 0);
	PM.printDetail(stdout, 0);
	PM.printThreads(stdout, 0);
	PM.printLegend(stdout);

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
double c1,c2,c3;
nsize = matrix.nsize;
#pragma omp parallel
#pragma omp for
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
		c1=0.0;
		for (k=0; k<nsize; k++){
		//	cx	c2=matrix.a2[i][k] * matrix.a2[j][k];
		//	cx	c3=matrix.b2[i][k] * matrix.b2[j][k];
		//	cx	c1=c1 + c2+c3;
		c1 += matrix.a2[i][k]*matrix.a2[j][k] + matrix.b2[i][k]*matrix.b2[j][k];
		}
		matrix.c2[i][j] = matrix.c2[i][j] + c1/(float)nsize;
	}
	}
}

// slow computing kernel
// on K,FX,Fugaku, -Nocl must be given in order to enable these directives
void slowkernel()
{
int i, j, k, nsize;
double c1,c2,c3;
nsize = matrix.nsize;

#pragma loop serial
	for (i=0; i<nsize; i++){
#pragma novector
#pragma nounroll
#pragma loop novector
#pragma loop nosimd
#pragma loop noswp
#pragma loop nounroll
	for (j=0; j<nsize; j++){
		c1=0.0;
#pragma novector
#pragma nounroll
#pragma loop novector
#pragma loop nosimd
#pragma loop noswp
#pragma loop nounroll
		for (k=0; k<nsize; k++){
		//	cx	c2=matrix.a2[i][k] * matrix.a2[j][k];
		//	cx	c3=matrix.b2[i][k] * matrix.b2[j][k];
		//	cx	c1=c1 + c2+c3;
		c1 += matrix.a2[i][k]*matrix.a2[j][k] + matrix.b2[i][k]*matrix.b2[j][k];
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
