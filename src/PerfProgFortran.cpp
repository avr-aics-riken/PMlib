#ifdef _PM_WITHOUT_MPI_
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <string>
#include <PerfMonitor.h>

#include "stdlib.h"

#if !defined(_WIN32) && !defined(WIN32)
#include <unistd.h> // for gethostname() of FX10/K
#endif

using namespace pm_lib;
PerfMonitor PM;

// only to prevent C++ name mangling
extern "C" {

/// PMlib Fortran インタフェイス
/// PMlibの初期化.
///
/// @param[in] init_nWatch 最初に確保する測定区間数。
///
/// @note   測定区間数 m_nWatch は不足すると内部で自動的に追加する
/// @note   Fortran とC++間のインタフェイスでは引数を省略する事はできないため、
///         PMlib C++の引数仕様と異なる事に注意
///
void f_pm_initialize_ (int& init_nWatch)
{
    int num_threads;
    int num_process;
    int my_rank;

#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_initialize_> init_nWatch=%d\n", init_nWatch);
#endif

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


/// PMlib Fortran インタフェイス
/// 測定区間にプロパティを設定.
///
///   @param[in] char* fc ラベルとなる character文字列
///   @param[in] int f_type  測定対象タイプ(COMM:通信, CALC:計算)
///   @param[in] int f_exclusive 排他測定フラグ(ディフォルトtrue)
///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
///        注意：fc_sizeはFortranコンパイラが自動的に追加してしまう引数
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
	fprintf(stderr, "<f_pm_setproperties_> fc=%s, f_type=%d, f_exclusive=%d, fc_size=%d\n", fc, f_type, f_exclusive, fc_size);
#endif
	if (s == "" || fc_size == 0) {
		fprintf(stderr, "<f_pm_setproperties> argument fc is empty(null)\n");
		PM_Exit(0);
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


/// PMlib Fortran インタフェイス
/// 測定区間スタート
///
///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
///        注意：fc_sizeはFortranコンパイラが自動的に追加してしまう引数
///
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


/// PMlib Fortran インタフェイス
/// 測定区間ストップ
///
///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
///   @param[in] fpt 「タスク」あたりの計算量(Flop)または通信量(Byte)
///   @param[in] tic  「タスク」実行回数
///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
///        注意：fc_sizeはFortranコンパイラが自動的に追加してしまう引数
///
///   @note  計算量または通信量をユーザが明示的に指定する場合は、
///          そのボリュームは１区間１回あたりでfpt*ticとして算出される
///   @note  Fortran PMlib インタフェイスでは引数を省略する事はできない。
///          引数値 fpt*tic が非0の場合はその数値が採用され、値が0の場合は
///          HWPC自動計測値が採用される。
///
void f_pm_stop_ (char* fc, double& fpt, unsigned& tic, int fc_size)
{
	std::string s=fc;

#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_stop_> fc=%s, fpt=%8.0lf, tic=%d, fc_size=%d\n", fc, fpt, tic, fc_size);
#endif
	if (s == "") {
		fprintf(stderr, "<f_pm_stop_> ");
		fprintf(stderr, "argument fc is empty(null)\n");
	}
	if (fc_size == 0) {
		fprintf(stderr, "<f_pm_stop_> ");
		fprintf(stderr, "argument fc_size is 0\n");
	}
	PM.stop(s, fpt, tic);
	return;
}

/// PMlib Fortran インタフェイス
/// 全プロセスの全測定結果情報をマスタープロセス(0)に集約
///
void f_pm_gather_ (void)
{
#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_gather_> \n");
#endif
	PM.gather();
	return;
}


/// PMlib Fortran インタフェイス
/// 測定結果の基本統計レポートを出力
///
///   @param[in] char* fc 出力ファイル名(character文字列)
///   @param[in] int fc_size  出力ファイル名の文字数
///        注意：fc_sizeはFortranコンパイラが自動的に追加してしまう引数
///
void f_pm_print_ (char* fc, int fc_size)
{
	FILE *fp;
	std::string s=fc;
#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_print_> fc=%s, fc_size=%d\n", fc, fc_size);
#endif
	std::string h;
	std::string u="user";
	char hostname[512];
	hostname[0]='\0';
	if (gethostname(hostname, sizeof(hostname)) != 0) {
		fprintf(stderr, "*** warning <f_pm_print_> can not obtain hostname\n");
		h="unknown";
	} else {
		h=hostname;
	}
	if (s == "" || fc_size == 0) {
		// filename is null. PMlib report is merged to stdout
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_print_> can not open: %s\n", fc);
			fp=stdout;
		}
	}

	PM.print(fp, h, u);
	return;
}

/// PMlib Fortran インタフェイス
/// MPIランク別詳細レポート、HWPC詳細レポートを出力。 非排他測定区間も出力
///
///   @param[in] char* fc 出力ファイル名(character文字列)
///   @param[in] legend  HWPC 記号説明の表示(0:表示する、1:表示しない)
///   @param[in] int fc_size  出力ファイル名の文字数
///        注意：fc_sizeはFortranコンパイラが自動的に追加してしまう引数
///
///   @note  Fortran とC++間のインタフェイスでは引数を省略する事はできないため、
///          PMlib C++の引数仕様と異なる事に注意
///
void f_pm_printdetail_ (char* fc, int& legend, int fc_size)
{
	FILE *fp;
	std::string s=fc;
#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_printdetail_> fc=%s, legend=%d, fc_size=%d \n", fc, legend, fc_size);
#endif

	if (s == "" || fc_size == 0) {
		// filename is null. PMlib report is merged to stdout
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_printdetail_> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	PM.printDetail(fp, legend);
	return;
}


/// PMlib Fortran インタフェイス
/// プロセスグループ単位でのMPIランク別詳細レポート、HWPC詳細レポート出力
///
///   @param[in] char* fc ラベルとなる character文字列
///   @param[in] p_group  MPI_Group型 groupのgroup handle
///   @param[in] p_comm   MPI_Comm型 groupに対応するcommunicator
///   @param[in] pp_ranks int**型 groupを構成するrank番号配列へのポインタ
///   @param[in] group    int型 プロセスグループ番号
///   @param[in] legend   int型 HWPC 記号説明の表示 (0:表示する、1:表示しない)
///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
///        注意：fc_sizeはFortranコンパイラが自動的に追加してしまう引数
///
///   @note  Fortran とC++間のインタフェイスでは引数を省略する事はできないため、
///          PMlib C++の引数仕様と異なる事に注意
///          MPI_Group, MPI_Comm型は呼び出すFortran側では integer 型である
///
void f_pm_printgroup_ (char* fc, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int& group, int& legend, int fc_size)

{
	FILE *fp;
	std::string s=fc;
#ifdef DEBUG_FORT
	fprintf(stderr, "<f_pm_printdetail_> fc=%s, group=%d, legend=%d, fc_size=%d \n", fc, group, legend, fc_size);
#endif

	if (s == "" || fc_size == 0) {
		// filename is null. PMlib report is merged to stdout
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_printgroup_> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	PM.printGroup(fp, p_group, p_comm, pp_ranks, group, legend);

	return;
}


}

