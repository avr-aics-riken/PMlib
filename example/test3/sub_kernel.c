#include <mpi.h>
#include <stdio.h>
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#define MATSIZE 1000
struct {
	int nsize;
	float a2[MATSIZE][MATSIZE]; /* address of matrix.a2 == &matrix.a2[0][0] */
	float b2[MATSIZE][MATSIZE];
	float c2[MATSIZE][MATSIZE];
} matrix;


void set_array()
{
int i, j, nsize;
nsize = matrix.nsize = MATSIZE;
//	printf("<set_array> nsize=%d \n", nsize);
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

void sub_kernel()
{
int i, j, k, nsize;
float c1,c2,c3;
nsize = matrix.nsize = MATSIZE;
//	printf("<sub_kernel> computing. nsize=%d\n", nsize);
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

