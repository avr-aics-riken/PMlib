#include <PerfMonitor.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <math.h>
#include <string>
#include <sstream>
using namespace pm_lib;

#define MATSIZE 1000
//	#define MATSIZE 500
//	#define MATSIZE 100
int nsize;
struct matrix {
	int nsize;
	float a2[MATSIZE][MATSIZE];
	float b2[MATSIZE][MATSIZE];
	float c2[MATSIZE][MATSIZE];
} matrix;

float somekernel();
void set_array();
void spacer();
extern	void merge_and_report(FILE* fp);
int my_id, npes, num_threads;
PerfMonitor PM;
#pragma omp threadprivate(PM)

int main (int argc, char *argv[])
{
	double flop_count, byte_count, dsize=MATSIZE;
	double t1, t2;
	float x;
	int i, j;
	int icomm=0, icalc=1;
	int i_thread;

	//	my_id=0;
	//	npes=1;
    MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);
	MPI_Barrier(MPI_COMM_WORLD);

	// initialize should be called inside parallel region
	//	PM.initialize(5);

#pragma omp parallel private(x)
{
	PM.initialize(5);

	set_array();
	for (int i=0; i<3; i++)
	{
	PM.start("Section-A");
	x = somekernel();
	PM.stop ("Section-A");
	}
}
	PM.start("Section-B");
	x = somekernel();
	PM.stop ("Section-B");

	//	PM.report(stdout);
	//	pm_lib::merge_and_report(stdout);
	merge_and_report(stdout);

	MPI_Finalize();
	return 0;
	//	exit(0);
}


void set_array()
{
int i, j, nsize;
matrix.nsize = MATSIZE ;
nsize = matrix.nsize;
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
	matrix.a2[i][j] = sin( (float)j/(float)nsize );
	matrix.b2[i][j] = cos( (float)j/(float)nsize );
	matrix.c2[i][j] = 0.0;
	}
	}
}

float somekernel() // any computing kernel
{
//	int nsize;
int nsize;
float x, c0,c1,c2,c3;
nsize = matrix.nsize;
	x=0.0;

//	#pragma omp parallel for private(c0,c1,c2,c3) shared(nsize) reduction(+:x)
	for (int i=0; i<nsize; i++){
		c0=0.0;
		for (int j=0; j<nsize; j++){
		c1=0.0;
		for (int k=0; k<nsize; k++){
			c2=matrix.a2[j][k] * matrix.a2[j][k];
			c3=matrix.b2[j][k] * matrix.b2[j][k];
			c1=c1 + c2+c3;
		}
		c0=c0 + c1;
		}
		x=x+c0;
	}
	return(x);
	//buggy. Intel C++ iteration variables i,j,k must be informed as private.
	//	int i,j,k;
	//	#pragma omp parallel for private(c0,c1,c2,c3,i,j,k) shared(x,nsize)
}

// add some meaningless space, for easier debug with visualization package
void spacer()
{
	//	for (int i=0; i<10; i++){ set_array(); }
}

