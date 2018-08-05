#include <PerfMonitor.h>
#include <omp.h>
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
int my_id, npes, num_threads;
PerfMonitor PM;
#pragma omp threadprivate(PM)

int main (int argc, char *argv[])
{
	double flop_count, byte_count, dsize=MATSIZE;
	double t1, t2;
	float x;
	int i, j, num_threads;
	int icomm=0, icalc=1;
	int i_thread;

    std::string comments;
    std::ostringstream ouch; // old C++ notation

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

	num_threads  = omp_get_max_threads();

#pragma omp parallel private(i,i_thread,flop_count,x)
{
	i_thread = omp_get_thread_num();
	fprintf(stderr, "<main> my_id=%d, i_thread=%d\n", my_id, i_thread);

	PM.initialize(5);

	set_array();

	flop_count = (100.0 + (double)i_thread)*10.0;	// for debug purpose

	PM.setProperties("Section-A", PerfMonitor::CALC);
	PM.setProperties("Section-B", PerfMonitor::CALC);

	PM.start("Section-A");
	x = somekernel();
	PM.stop ("Section-A", flop_count, 1);
	fprintf(stderr, "\t somekernel rank:%d thread:%d computed x=%f \n",
		my_id, i_thread, x);

	PM.start("Section-B");
	PM.stop ("Section-B", flop_count, 1);

	PM.mergeThreads();
}

	fprintf(stderr, "\n *** Now starting print sections. ***\n");

	PM.print(stdout,"","", 1);
	PM.printDetail(stdout, 0, 1);
	PM.printThreads(stdout, 0, 1);
	PM.printLegend(stdout);
	//	PM.postTrace();

	MPI_Finalize();
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
}

float somekernel() // any computing kernel
{
int nsize;
float x, c0,c1,c2,c3;
nsize = matrix.nsize;
	x=0.0;

#pragma omp parallel for private(c0,c1,c2,c3) shared(nsize) reduction(+:x)
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
}

// add some meaningless space, for easier debug with visualization package
void spacer()
{
	//	for (int i=0; i<10; i++){ set_array(); }
}
