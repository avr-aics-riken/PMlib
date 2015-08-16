#ifdef _PM_WITHOUT_MPI_
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string>
#ifdef _OPENMP
	#include <omp.h>
	char parallel_mode[] = "Hybrid";
	//	char parallel_mode[] = "OpenMP";
#else
	char parallel_mode[] = "Serial";
	//	char parallel_mode[] = "FlatMPI";
#endif
#include <PerfMonitor.h>
using namespace pm_lib;
PerfMonitor PM;
extern "C" void set_array(), sub_kernel();

int main (int argc, char *argv[])
{
	int my_id, npes, num_threads;
	int i, j, loop;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);

#ifdef _OPENMP
	char* c_env = std::getenv("OMP_NUM_THREADS");
	if (c_env == NULL) omp_set_num_threads(1);
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif

	PM.initialize();
	PM.setParallelMode(parallel_mode, num_threads, npes);
	PM.setRankInfo(my_id);
	(void)PM.getVersionInfo();

	PM.setProperties("First location", PerfMonitor::CALC);
	PM.setProperties("Second location", PerfMonitor::CALC);

	if(my_id == 0) fprintf(stderr, "<main> starting the First location.\n");
	set_array();
	loop=3;
	PM.start("First location");
	for (i=1; i<=loop; i++){
		sub_kernel();
	}
	PM.stop ("First location");

	if(my_id == 0) fprintf(stderr, "<main> starting the Second location.\n");
	PM.start("Second location");
	sub_kernel();
	PM.stop ("Second location");

	PM.gather();
	PM.print(stdout, "", "Mr. Bean");
	PM.printDetail(stdout);

	MPI_Finalize();
	return 0;
}


