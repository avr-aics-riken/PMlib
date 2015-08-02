#ifdef _PM_WITHOUT_MPI_
#include <iostream>
int main (int argc, char *argv[])
{
	std::cout << "This test program is aimed for MPI group." <<  std::endl;
	std::cout << "Skipping the test..." <<  std::endl;
	return 0;
}

#else

#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <string>
#ifdef _OPENMP
	#include <omp.h>
	char parallel_mode[] = "Hybrid";
	//	char parallel_mode[] = "OpenMP";
#else
	char parallel_mode[] = "FlatMPI";
	//	char parallel_mode[] = "Serial";
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
	PM.setParallelMode(parallel_mode, num_threads, num_process);
	PM.setRankInfo(my_id);
	(void)PM.getVersionInfo();

	PM.setProperties("First block", PerfMonitor::CALC);
	PM.setProperties("Second block", PerfMonitor::CALC);

	if(my_id == 0) {
		fprintf(stderr, "<main> starting the WORLD (default) communicator.\n");
	}

// Computing as a single group (default)

	set_array();
	loop=3;
	PM.start("First block");
	for (i=1; i<=loop; i++){
		sub_kernel();
	}
	PM.stop ("First block");

// Create some groups for testing.
	MPI_Group new_group1;
	MPI_Comm new_comm1;
	int i_group1;
	int new_size1;
	int* p1_my_id;

	new_size1 = std::max(1,num_process/2);
	p1_my_id = new int[new_size1];
	for (i=0; i<new_size1; i++) { p1_my_id[i] = i; }
	MPI_Group_incl(my_group, new_size1, p1_my_id, &new_group1);
	MPI_Comm_create(MPI_COMM_WORLD, new_group1, &new_comm1);

	PM.start("Second block");
	if(my_id<new_size1) {
		stream_copy();
	} else {
		stream_triad();
	}
	PM.stop ("Second block");

	PM.gather();
	PM.print(stdout, "", "Mr. Bean");
	PM.printDetail(stdout);


	PM.printGroup(stdout, new_group1, new_comm1, p1_my_id, 77);

	MPI_Comm new_comm2;
	MPI_Group new_group2;
	int i_group2;
	int new_size2;
	int* p2_my_id;

/*
  void PerfMonitor::printGroup(FILE* fp, int p_group, int p_comm, int* pp_ranks)

  /// プロセスグループ単位でのHWPC計測結果、MPIランク別詳細レポート出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] p_group MPI_Group型 groupのgroup handle
  ///   @param[in] p_comm  MPI_Comm型 groupに対応するcommunicator
  ///   @param[in] pp_ranks int**型 groupを構成するrank番号配列へのポインタ
  ///
  ///   @note プロセスグループは呼び出しプログラムが定義する
  ///   @note MPI_Group型, MPI_Comm型は int *型とコンパチ
  ///
*/

	MPI_Finalize();
	return 0;
}

#endif
