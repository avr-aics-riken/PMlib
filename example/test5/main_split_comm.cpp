#ifdef _PM_WITHOUT_MPI_
#include <iostream>
int main (int argc, char *argv[])
{
    std::cout << "This test program is aimed for MPI split." <<  std::endl;
    std::cout << "Skipping the test..." <<  std::endl;
    return 0;
}
#else

#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <list>
#ifdef _OPENMP
	#include <omp.h>
#endif
#include <PerfMonitor.h>
using namespace pm_lib;
PerfMonitor PM;

extern "C" void stream_copy();
extern "C" void stream_triad();
extern "C" void set_array(), sub_kernel();

int main (int argc, char *argv[])
{
	int my_id, num_process, num_threads;
	int i, j, k, loop;

	double flop_count, bandwidth_count, dsize;
	double t1, t2;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &num_process);

#ifdef _OPENMP
	char* c_env = std::getenv("OMP_NUM_THREADS");
	if (c_env == NULL) {
		fprintf (stderr, "undefined OMP_NUM_THREADS is now set as 1\n");
		omp_set_num_threads(1);
	}
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif

	PM.initialize();

	PM.setProperties("section-1", PerfMonitor::CALC);
	PM.setProperties("section-2", PerfMonitor::CALC);


	MPI_Comm new_comm;
	int icolor, key;

	int ncolors = 2;
	icolor = my_id*ncolors / num_process;
	if(my_id == 0) {
		fprintf(stderr, "testing MPI_Comm_split. ncolors=%d\n", ncolors);
	}

	key = my_id;
	MPI_Comm_split(MPI_COMM_WORLD, icolor, key, &new_comm);

	//	int id, size;
	//	MPI_Comm_rank(new_comm, &id);	// new rank id within new_comm
	//	MPI_Comm_size(new_comm, &size);	// the size of new_comm
	//	fprintf(stderr, "\t my_id=%d, icolor=%d, id=%d\n", my_id,icolor,id);


	PM.start("section-1");
	set_array();
	PM.stop ("section-1");

	//	sub_kernel();

	PM.start("section-2");
	if(icolor == 1) {
		stream_copy();
	} else {
		stream_triad();
	}
	PM.stop ("section-2");

	PM.gather();
	PM.print(stdout, "", "");
	PM.printDetail(stdout);
	PM.printComm (stdout, new_comm, icolor, key);
	//	PM.print(stdout, "", "", 1);
	//	PM.printDetail(stdout, 0, 1);
	//	PM.printComm (stdout, new_comm, icolor, key, 0, 1);

	MPI_Finalize();
	return 0;
}

#endif

