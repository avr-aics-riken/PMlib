#ifdef _PM_WITHOUT_MPI_
#else
#include <mpi.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
//	#include <PerfMonitor.h>
#include <string>
//	using namespace pm_lib;
//	PerfMonitor PM;
#include <PerfMonitor.h>

#include "stdlib.h"

#if !defined(_WIN32) && !defined(WIN32)
#include <unistd.h> // for gethostname() of FX10/K
#endif

using namespace pm_lib;
PerfMonitor PM;

// only to prevent C++ name mangling
extern "C" {

    /// private : Fortran 2 C++ interface function prototype
    ///
    ///   @param[in] char* fc character文字列
    ///   @param[in] int fc_size  character文字列の長さ（文字数）
    ///
    ///   @note Fortran からの呼び出し:  call f_pm_call_cpp (fc//char(0))
    ///   character文字列 fc の最後にnull '\0'を追加すると処理が安全になる
    ///   第2引数 fc_size はFortranコンパイラが自動的に追加してしまう
    ///
void f_pm_call_cpp_ (char* fc, int fc_size)
{
	fprintf(stderr, "<f_pm_call_cpp_> is called with arg: %s", fc);
}

    /// PMlibの初期化.
    /// 測定区間数分の測定時計を準備.
    /// 最初にinit_nWatch区間分を確保し、不足したら動的にinit_nWatch追加する
    /// 全計算時間用測定時計をスタート.
    ///
    /// @param[in] （引数はオプション） init_nWatch 最初に確保する測定区間数
    ///
    /// @note 測定区間数 m_nWatch は不足すると動的に増えていく
    ///
//	void f_pm_initialize_ (int init_nWatch=100)
void f_pm_initialize_ (int& init_nWatch)
{
    int num_threads;
    int num_process;
    int my_rank;

#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_initialize_> init_nWatch=%d\n", init_nWatch);
#endif

	//	PM.initialize();
	PM.initialize(init_nWatch);

#ifdef _PM_WITHOUT_MPI_
	#ifdef _OPENMP
   	char parallel_mode[] = "OpenMP";
	#else
   	char parallel_mode[] = "Serial";
	#endif
    num_process=1;
    my_rank=0;
#else
	#ifdef _OPENMP
   	char parallel_mode[] = "Hybrid";
	#else
   	char parallel_mode[] = "FlatMPI";
	#endif
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
#endif

#ifdef _OPENMP
	char* c_env = std::getenv("OMP_NUM_THREADS");
	if (c_env == NULL) omp_set_num_threads(1);	// if not defined, set it as 1
	num_threads  = omp_get_max_threads();
	//	PM.num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif

	PM.setRankInfo(my_rank);
	PM.setParallelMode(parallel_mode, num_threads, num_process);
	return;
}


    /// 測定区間にプロパティを設定.
    ///
    ///   @param[in] char* fc ラベルとなる character文字列
    ///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
    ///   @param[in] int f_type  測定対象タイプ(COMM:通信, CALC:計算)
    ///   @param[in] int f_exclusive 排他測定フラグ(ディフォルトtrue)
    ///
    ///   @note ラベルは測定区間を識別するために用いる。
    ///   各labelに対応したキー番号 key は各ラベル毎に内部で自動生成する
    ///
void f_pm_setproperties_ (char* fc, int& f_type, int& f_exclusive, int fc_size)
{
	std::string s=fc;
	bool exclusive;
    PerfMonitor::Type arg_type; /// 測定対象タイプ from PerfMonitor.h

#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_setproperties_> fc=%s, fc_size=%d, f_type=%d, f_exclusive=%d\n", fc, fc_size, f_type, f_exclusive);
#endif
	if (s == "") {
		fprintf(stderr, "<f_pm_setproperties> ");
		fprintf(stderr, "argument fc is empty(null)\n");
	}
	if (fc_size == 0) {
		fprintf(stderr, "<f_pm_setproperties> ");
		fprintf(stderr, "argument fc_size is 0\n");
	}
	if (f_exclusive == 1) {
		exclusive=true;
	} else if (f_exclusive == 2) {
		exclusive=false;
	} else {
		fprintf(stderr, "<f_pm_setproperties> ");
		fprintf(stderr, "argument f_exclusive is invalid: %u\n", f_exclusive);
		PM_Exit(0);
	}
	//	PM.setProperties(s, f_type, exclusive);
	if (f_type == 0) {
		arg_type = PM.COMM;
	} else if (f_type == 1) {
		arg_type = PM.CALC;
	} else {
		fprintf(stderr, "<f_pm_setproperties> ");
		fprintf(stderr, "argument f_type is invalid: %u\n", f_type);
		PM_Exit(0);
	}
	PM.setProperties(s, arg_type, exclusive);
	return;
}


void f_pm_start_ (char* fc, int fc_size)
{
	std::string s=fc;

#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_start_> fc=%s, fc_size=%d\n", fc, fc_size);
#endif
	if (s == "") {
		fprintf(stderr, "<f_pm_start_> ");
		fprintf(stderr, "argument fc is empty(null)\n");
	}
	if (fc_size == 0) {
		fprintf(stderr, "<f_pm_start_> ");
		fprintf(stderr, "argument fc_size is 0\n");
	}
	PM.start(s);
	return;
}


void f_pm_stop_ (char* fc, int fc_size)
{
	std::string s=fc;

#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_stop_> fc=%s, fc_size=%d\n", fc, fc_size);
#endif
	if (s == "") {
		fprintf(stderr, "<f_pm_stop_> ");
		fprintf(stderr, "argument fc is empty(null)\n");
	}
	if (fc_size == 0) {
		fprintf(stderr, "<f_pm_stop_> ");
		fprintf(stderr, "argument fc_size is 0\n");
	}
	PM.stop(s);
	return;
}

void f_pm_gather_ (void)
{
#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_gather_> \n");
#endif
	PM.gather();
	return;
}


void f_pm_print_ (void)
{
#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_print_> \n");
#endif
	std::string h;
	std::string u="user";
  char hostname[512];
  memset(hostname, 0x00, sizeof(char)*512);
	if (gethostname(hostname, sizeof(hostname)) != 0) {
		fprintf(stderr, "<f_pm_print_> hostname could not be obtained\n");
		h="unknown";
	} else {
		h=hostname;
	}

	PM.print(stdout, h, u);
	return;
}

void f_pm_printdetail_ (void)
{
#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_printdetail_> \n");
#endif
	PM.printDetail(stdout);
	return;
}
  

}

