#include <string>
#include <math.h>
#include <stdio.h>
void init2d();
void mxm2d();
	//	#include <PerfMonitor.h>
	//	using namespace pm_lib;
	//	PerfMonitor PM;

#define MATSIZE 1000
int nsize;
struct matrix {
	int nsize;
	float a2[MATSIZE][MATSIZE];
	float b2[MATSIZE][MATSIZE];
	float c2[MATSIZE][MATSIZE];
} matrix;

int main (int argc, char *argv[])
{
	//	PM.initialize();
	//	PM.start("A:init2d");
	init2d();
	//	PM.stop ("A:init2d");
	//	PM.start("B:mxm2d");
	mxm2d();
	//	PM.stop ("B:mxm2d");
	//	PM.print(stdout, "", "", 0);
	//	PM.printDetail(stdout);
	//	PM.printThreads(stdout, 0, 0);
	printf("something was computed... %f\n",c2[0][0]);
	return 0;
}


void init2d()
{
	int i, j, nsize;
	matrix.nsize = MATSIZE;
	nsize = matrix.nsize;
	//	//	#pragma omp parallel for private(i,j)
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
	matrix.a2[i][j] = sin((float)j/(float)nsize);
	matrix.b2[i][j] = cos((float)j/(float)nsize);
	matrix.c2[i][j] = 0.0;
	}
	}
}

void mxm2d()
{
	int i, j, k, nsize;
	float c1,c2,c3;
	nsize = matrix.nsize;
	//	//	#pragma omp parallel for private(i,j,k,c1)
	for (i=0; i<nsize; i++){
	for (j=0; j<nsize; j++){
		c1=0.0;
		for (k=0; k<nsize; k++){
		c1=c1 + matrix.a2[k][i] * matrix.b2[j][k];
		}
		matrix.c2[j][i] = c1;
	}
	}
}


