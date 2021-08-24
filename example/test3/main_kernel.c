#include <pmlib_api_C.h>
#include <stdio.h>
#include <math.h>
#ifdef _OPENMP
	#include <omp.h>
#endif

extern void set_array(), sub_kernel();

int main (int argc, char *argv[])
{
	int my_id, npes;
	int i, j, loop;

	//	MPI_Init(&argc, &argv);
	//	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	//	MPI_Comm_size(MPI_COMM_WORLD, &npes);


	int nsections=5;
	C_pm_initialize(nsections);

	if(my_id == 0) fprintf(stderr, "<main> starting the First location.\n");
	set_array();
	loop=3;
	for (i=1; i<=loop; i++){
		C_pm_start("First location");
		sub_kernel();
		C_pm_stop ("First location");
	}

	if(my_id == 0) fprintf(stderr, "<main> starting the Second location.\n");
	C_pm_start("Second location");
	sub_kernel();
	C_pm_stop ("Second location");

	C_pm_report("");	// report to stdout by default

	//	MPI_Finalize();
	return 0;
}


