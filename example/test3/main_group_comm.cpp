#ifdef DISABLE_MPI
#include <iostream>
int main (int argc, char *argv[])
{
	using namespace std;
	cout << "This test program is aimed for MPI group management " <<  endl;
	cout << "such as MPI_Group_incl(), MPI_Comm_create(), etc." << endl;
	cout << "To run this test, PMlib must be built with MPI." <<  endl;
	cout << "Skipping this test..." <<  endl;
	return 0;
}

#else
#include <PerfMonitor.h>
//	#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <string>
#ifdef _OPENMP
	#include <omp.h>
#endif
using namespace pm_lib;
PerfMonitor PM;

extern "C" void stream_copy();
extern "C" void stream_triad();
extern "C" void set_array(), sub_kernel();

int main (int argc, char *argv[])
{

	int my_id, num_process, num_threads;
	int i, j, loop;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &num_process);

	MPI_Group my_group;
	MPI_Comm_group(MPI_COMM_WORLD, &my_group);

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

	PM.setProperties("1st section", PerfMonitor::CALC);
	PM.setProperties("2nd section", PerfMonitor::CALC);

	if(my_id == 0) {
		fprintf(stderr, "<main> starting the WORLD (default) communicator.\n");
	}

// Create some groups for testing.
	MPI_Group new_group1, new_group2;
	MPI_Comm new_comm1, new_comm2;
	int i_group1, i_group2;
	int new_size1, new_size2;
	int *p1_my_id, *p2_my_id;

// 1st group
	new_size1 = std::max(1,num_process/2);
	p1_my_id = new int[new_size1];
	for (i=0; i<new_size1; i++) { p1_my_id[i] = i; }
	MPI_Group_incl(my_group, new_size1, p1_my_id, &new_group1);
	MPI_Comm_create(MPI_COMM_WORLD, new_group1, &new_comm1);

// 2nd group
	new_size2 = num_process - new_size1;
	p2_my_id = new int[new_size2];
	for (i=0; i<new_size2; i++) { p2_my_id[i] = new_size1 + i; }
	MPI_Group_incl(my_group, new_size2, p2_my_id, &new_group2);
	MPI_Comm_create(MPI_COMM_WORLD, new_group2, &new_comm2);


	set_array();
// using the world communicator (default)
	loop=1;
	PM.start("1st section");
	for (i=1; i<=loop; i++){
		sub_kernel();
	}
	PM.stop ("1st section");

// using the group communicators
	PM.start("2nd section");
	if(my_id<new_size1) {
		stream_copy();
	} else if(my_id<new_size1+new_size2) {
		stream_triad();
	} else {
		;
	}
	PM.stop ("2nd section");


	PM.gather();
	PM.print(stdout, "", "Mr. Bean");
	PM.printDetail(stdout);
	PM.printGroup(stdout, new_group1, new_comm1, p1_my_id, 1);
	PM.printGroup(stdout, new_group2, new_comm2, p2_my_id, 2, 1);

	MPI_Finalize();
	return 0;
}
#endif
