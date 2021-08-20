//
//	Remark that FCC -Kfast would skip all the computations in this kernel
//	judging that they are not used in the end.
//	So don't put -Kfast for testing PMlib
//
#define DISABLE_MPI
#include <PerfMonitor.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <math.h>
#include <string>
#include <sstream>

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
int my_id, npes, num_threads;

pm_lib::PerfMonitor PM;
pm_lib::PerfReport  PR;
// To generate report including the sections inside of parallel regions,
// the PerfReport class report() must be used instead of PerfMonitor report()
// like this example

#pragma omp threadprivate(PM)

int main (int argc, char *argv[])
{
	double flop_count, byte_count, dsize=MATSIZE;
	double t1, t2;
	float x;
	int i, j, num_threads;
	int icomm=0, icalc=1;
	int i_thread;

	my_id=0;
	npes=1;
#ifdef _OPENMP
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif


#pragma omp parallel private(x)
{
	PM.initialize(5);

	set_array();

	PM.start("Section-A");
	x = somekernel();
	PM.stop ("Section-A");
}

	for (int i=0; i<2; i++)
	{
	PM.start("Section-B");
	x = somekernel();
	PM.stop ("Section-B");
	}

	//	PM.report(stdout);
	PR.report(stdout);

	return 0;
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
	//	printf("<set_array> routine initialized data for nsize=%d \n", nsize);
}

float somekernel() // any computing kernel
{
//	int nsize;
int nsize;
float x, c0,c1,c2,c3;
nsize = matrix.nsize;
	x=0.0;

	//buggy.
	//	Intel C++ iteration variables i,j,k must be informed as private.
	//	int i,j,k;
	//	#pragma omp parallel for private(c0,c1,c2,c3,i,j,k) ...
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
	//	printf("<somekernel> routine computed data for nsize=%d \n", nsize);
	return(x);
}

// add some meaningless space, for easier debug with visualization package
void spacer()
{
	//	for (int i=0; i<10; i++){ set_array(); }
}
