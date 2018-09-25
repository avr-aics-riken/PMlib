/*
###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2018 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2018 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################
 */

#ifdef DISABLE_MPI
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


#ifdef _OPENMP
// Worksharing parallel construct is generally supported.
// Non-worksharing parallel construct is limited to Intel and PGI only.

#if defined (__INTEL_COMPILER)
	#pragma omp threadprivate(PM)

#elif defined (__PGI)
	#pragma omp threadprivate(PM)

	#if defined (FORCE_CXX_MAIN)
	// PGI mixed Fortran and C++ non-worksharing openmp parallel construct
	// needs this small main driver, i.e. PGI's undocumented restrictions.
	// This main driver handles threadprivate class member variables
	// passed across Fortran and C++
	extern "C" void fortmain_(void);
	extern "C" void main(void);
	void main(void)
	{
		(void) fortmain_();
	}
	#endif

#endif

	//#elif defined (__FUJITSU)
	// FX100 and K compiler does not support threadprivate class instance
	//#elif defined (__clang__)
	// Clang does not support OpenMP
	//#elif defined (__GNUC__)
	// g++ version 5.5 causes compile error against threadprivate class instance
	//#else
	// Other compilers to be tested
#endif


// Fortran interface should avoid C++ name space mangling, thus this extern.
extern "C" {

/// PMlib Fortran インタフェイス
/// PMlibの初期化.
///
/// @param[in] init_nWatch 最初に確保する測定区間数。
///
/// @note   測定区間数 m_nWatch は不足すると内部で自動的に追加する
/// @note   Fortran インタフェイスでは引数を省略する事はできない。
///         PMlib C++の引数仕様と異なる事に注意
///
void f_pm_initialize_ (int& init_nWatch)
{

    int num_threads;
    int num_process;
    int my_rank;

#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_initialize_> init_nWatch=%d\n", init_nWatch);
#endif

	PM.initialize(init_nWatch);

#ifdef DISABLE_MPI
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
	//	char* c_env = std::getenv("OMP_NUM_THREADS");
	//	if (c_env == NULL) omp_set_num_threads(1);
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif

	PM.setParallelMode(parallel_mode, num_threads, num_process);
	return;
}


/// PMlib Fortran インタフェイス
/// 測定区間にプロパティを設定.
///
///   @param[in] char* fc ラベルとなる character文字列
///   @param[in] int f_type  測定対象タイプ(0:COMM:通信, 1:CALC:計算)
///   @param[in] int f_exclusive 排他測定フラグ(0:false, 1:true)
///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
///
///   @note ラベルは測定区間を識別するために用いる。
///   		各ラベル毎に対応した区間番号は内部で自動生成する
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_setproperties_ (char* fc, int& f_type, int& f_exclusive, int fc_size)
{
	//	Note on fortran character 2 C++ string
	//	Although the auto appended fc_size argument value is correct,
	//	fortran character string is not terminated with NUL
	//	A simple conversion such as below is not safe.
	//		std::string s=fc;
	//	So, we do explicit string conversion here...
	std::string s=std::string(fc,fc_size);
	bool exclusive;
    PerfMonitor::Type arg_type; /// 測定対象タイプ from PerfMonitor.h

#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_setproperties_> fc=%s, f_type=%d, f_exclusive=%d, fc_size=%d\n", s.c_str(), f_type, f_exclusive, fc_size);
#endif
	if (s == "" || fc_size == 0) {
		fprintf(stderr, "<f_pm_setproperties> argument fc is (null). The call is ignored.\n");
		return;
	}
	if (f_exclusive == 1) {
		exclusive=true;
	} else if ((f_exclusive == 0)||(f_exclusive == 2)) {
		exclusive=false;
	} else {
		fprintf(stderr, "<f_pm_setproperties> argument f_exclusive is invalid: %u . The call is ignored.\n", f_exclusive);
		return;
	}
	//	PM.setProperties(s, f_type, exclusive);
	if (f_type == 0) {
		arg_type = PM.COMM;
	} else if (f_type == 1) {
		arg_type = PM.CALC;
	} else {
		fprintf(stderr, "<f_pm_setproperties> argument f_type is invalid: %u . The call is ignored. \n", f_type);
		return;
	}
	PM.setProperties(s, arg_type, exclusive);
	return;
}


/// PMlib Fortran インタフェイス
/// 測定区間スタート
///
///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_start_ (char* fc, int fc_size)
{
	std::string s=std::string(fc,fc_size);

#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_start_> fc=%s, fc_size=%d\n", s.c_str(), fc_size);
#endif
	if (s == "") {
		fprintf(stderr, "<f_pm_start_> argument fc is empty(null)\n");
		return;
	}
	if (fc_size == 0) {
		fprintf(stderr, "<f_pm_start_> argument fc_size is 0\n");
		return;
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
///
///   @note  計算量または通信量をユーザが明示的に指定する場合は、
///          そのボリュームは１区間１回あたりでfpt*ticとして算出される
///   @note  Fortran PMlib インタフェイスでは引数を省略する事はできない。
///          引数値 fpt*tic が非0の場合はその数値が採用され、値が0の場合は
///          HWPC自動計測値が採用される。
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_stop_ (char* fc, double& fpt, unsigned& tic, int fc_size)
{
	std::string s=std::string(fc,fc_size);

#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_stop_> fc=%s, fpt=%8.0lf, tic=%d, fc_size=%d\n", s.c_str(), fpt, tic, fc_size);
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
/// 測定区間リセット
///
///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
///   @param[in] int fc_size  character文字列ラベルの長さ（文字数）
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_reset_ (char* fc, int fc_size)
{
	std::string s=std::string(fc,fc_size);

#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_reset_> fc=%s, fc_size=%d\n", s.c_str(), fc_size);
#endif
	if (s == "") {
		fprintf(stderr, "<f_pm_reset_> argument fc is empty(null)\n");
		return;
	}
	if (fc_size == 0) {
		fprintf(stderr, "<f_pm_reset_> argument fc_size is 0\n");
		return;
	}
	PM.reset(s);
	return;
}


/// PMlib Fortran インタフェイス
/// 全測定区間リセット
///
///
void f_pm_resetall_ (void)
{
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_resetall_> \n");
#endif
	PM.resetAll();
	return;
}


/// PMlib Fortran インタフェイス
/// 全プロセスの全測定結果情報をマスタープロセス(0)に集約
///
void f_pm_gather_ (void)
{
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_gather_> \n");
#endif
	PM.gather();
	return;
}


/// PMlib Fortran インタフェイス
///  OpenMP並列処理されたPMlibスレッド測定区間のうち parallel regionから
///  呼び出された測定区間のスレッド測定情報をマスタースレッドに集約する。
///
void f_pm_mergethreads_ (void)
{
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_mergethreads_> \n");
#endif
	PM.mergeThreads();
	return;
}


/// PMlib Fortran インタフェイス
/// 測定結果の基本統計レポートを出力
///
///   @param[in] char* fc 出力ファイル名(character文字列)
///   @param[in] int psort 測定区間の表示順(0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int fc_size  出力ファイル名の文字数
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_print_ (char* fc, int &psort, int fc_size)
{

	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	#ifdef _OPENMP
	//	#pragma omp barrier
	//	#endif
	//	int my_rank;
    //	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	//	if (my_rank == 0) {
	//		fprintf(stderr, "\n<f_pm_print_> fc=%s, psort=%d, fc_size=%d\n", s.c_str(), psort, fc_size);
	//	}
#endif
	std::string h;
	std::string u="Fortran API";
	char hostname[512];
	hostname[0]='\0';
	if (gethostname(hostname, sizeof(hostname)) != 0) {
		fprintf(stderr, "*** warning <f_pm_print_> can not obtain hostname\n");
		h="unknown";
	} else {
		h=hostname;
	}

	int user_file;
	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_print_> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}
	if (psort != 0 && psort != 1) psort = 0;

	PM.print(fp, h, u, psort);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib Fortran インタフェイス
/// MPIランク別詳細レポート、HWPC詳細レポートを出力。 非排他測定区間も出力
///
///   @param[in] char* fc 出力ファイル名(character文字列)
///   @param[in] int legend  HWPC 記号説明の表示(0:なし、1:表示する)
///   @param[in] int psort 測定区間の表示順 (0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int fc_size  出力ファイル名の文字数
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printdetail_ (char* fc, int& legend, int &psort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printdetail_> fc=%s, legend=%d, psort=%d, fc_size=%d \n", s.c_str(), legend, psort, fc_size);
#endif

	int user_file;
	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_printdetail_> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}
	if (psort != 0 && psort != 1) psort = 0;

	PM.printDetail(fp, legend, psort);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib Fortran インタフェイス
/// 指定するプロセスのスレッド別詳細レポートを出力。
///
///   @param[in] char* fc	出力ファイル名(character文字列)
///   @param[in] int rank_ID	指定するプロセスのランク番号
///   @param[in] int psort 測定区間の表示順(0:経過時間順にソート、1:登録順で表示)
///   @param[in] int fc_size  出力ファイル名の文字数
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printthreads_ (char* fc, int &rank_ID, int &psort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printThreads_> fc=%s, rank_ID=%d, psort=%d, fc_size=%d \n", s.c_str(), rank_ID, psort, fc_size);
#endif

	int user_file;
	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_printThreads_> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}
	if (psort != 0 && psort != 1) psort = 0;

	PM.printThreads(fp, rank_ID, psort);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib Fortran インタフェイス
/// HWPC 記号の説明表示を出力。
///
///   @param[in] char* fc	出力ファイル名(character文字列)
///   @param[in] int fc_size  出力ファイル名の文字数
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printlegend_ (char* fc, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);

	int user_file;
	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_printdetail_> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}

	PM.printLegend(fp);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib Fortran インタフェイス
/// 指定するMPIプロセスグループ毎にMPIランク詳細レポートを出力
///
///   @param[in] char* fc 出力ファイル名(character文字列)
///   @param[in] MPI_Group型 p_group  MPIのgroup handle
///   @param[in] MPI_Comm型 p_comm   groupに対応するcommunicator
///   @param[in] int**型	pp_ranks groupを構成するrank番号配列へのポインタ
///   @param[in] int	group    プロセスグループ番号
///   @param[in] int	legend  HWPC 記号説明の表示(0:なし、1:表示する)
///   @param[in] int	psort 測定区間の表示順
///                       (0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int	fc_size  character文字列ラベルの長さ（文字数）
///
///   @note  MPI_Group, MPI_Comm型は呼び出すFortran側では integer 型である
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///   @note  HWPCを測定した計集結果があればそれも出力する
///
void f_pm_printgroup_ (char* fc, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int& group, int& legend, int &psort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printgroup_> fc=%s, group=%d, legend=%d, psort=%d, fc_size=%d \n", s.c_str(), group, legend, psort, fc_size);
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
	if (psort != 0 && psort != 1) psort = 0;

	PM.printGroup(fp, p_group, p_comm, pp_ranks, group, legend, psort);

	return;
}


/// PMlib Fortran インタフェイス
/// MPI_Comm_splitで作成するグループ毎にMPIランク詳細レポートを出力
///
///   @param[in] char*	fc	出力ファイル名(character文字列)
///   @param[in] MPI_Comm型 new_comm   groupに対応するcommunicator
///   @param[in] int	icolor   MPI_Comm_split()のカラー変数
///   @param[in] int	key      MPI_Comm_split()のkey変数
///   @param[in] int	legend  HWPC 記号説明の表示(0:なし、1:表示する)
///   @param[in] int	psort 測定区間の表示順
///                       (0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int	fc_size  character文字列ラベルの長さ（文字数）
///
///   @note  MPI_Group, MPI_Comm型は呼び出すFortran側では integer 型である
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printcomm_ (char* fc, MPI_Comm new_comm, int& icolor, int& key, int& legend, int& psort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printcomm_> fc=%s, new_comm=%d, icolor=%d, key=%d, legend=%d, psort=%d, fc_size=%d \n",
	//		s.c_str(), new_comm, icolor, key, legend, psort, fc_size);
#endif

	if (s == "" || fc_size == 0) {
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_printcomm_> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (psort != 0 && psort != 1) psort = 0;

	PM.printComm (fp, new_comm, icolor, key, legend, psort);

	return;
}


/// PMlib Fortran インタフェイス
/// 測定途中経過の状況レポートを出力（排他測定区間を対象とする）
///
///   @param[in] fc	出力ファイル名(character文字列)
///   @param[in] comments	任意のコメント(character文字列)
///   @param[in] psort 測定区間の表示順 (0:経過時間順に表示、1:登録順で表示)
///   @param[in] fc_size  fc文字列の長さ（文字数）
///   @param[in] comments_size  comments文字列の長さ（文字数）
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printprogress_ (char* fc, char* comments, int& psort, int fc_size, int comments_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printprogress_> fc=%s, comments=%s, psort=%d, fc_size=%d, comments_size=%d \n",
	//		s.c_str(), comments, psort, fc_size, comments_size);
#endif

	if (s == "" || fc_size == 0) {
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			fprintf(stderr, "*** warning <f_pm_printcomm_> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (psort != 0 && psort != 1) psort = 0;

	std::string s2=std::string(comments,comments_size);
	PM.printProgress (fp, s2, psort);

	return;
}


/// PMlib Fortran インタフェイス
/// ポスト処理用traceファイルの出力
///
void f_pm_posttrace_ (void)
{
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_posttrace_> \n");
#endif

	PM.postTrace ();

	return;
}


// PMlib Fortran インタフェイス終了
} // closing extern "C"

