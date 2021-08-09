/*
###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2020 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2020 Research Institute for Information Technology(RIIT), Kyushu University.
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
#include <stdlib.h>
#include <unistd.h>
#include "PerfMonitor.h"

#ifndef _OPENMP
using namespace pm_lib;
PerfMonitor PM;

#else
// compilers support OpenMP through different implementations

#if defined (__INTEL_COMPILER)	|| \
	defined (__CLANG_FUJITSU)

	using namespace pm_lib;
	PerfMonitor PM;
	#pragma omp threadprivate(PM)

#elif defined (__FUJITSU)
	//  Fujitsu traditional C++ without threadprivate class support
	using namespace pm_lib;
	PerfMonitor PM;

#elif defined (__GXX_ABI_VERSION)
	// GNU g++ is not fully compliant with OpenMP threadprivate class support
	// this is a work around. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=27557
	using namespace pm_lib;
	extern PerfMonitor PM;
	#pragma omp threadprivate(PM)
	PerfMonitor PM;

#elif defined (__PGI)
	using namespace pm_lib;
	PerfMonitor PM;
	#pragma omp threadprivate(PM)
	#if defined (FORCE_CXX_MAIN)
		// A small C++ main driver to handle OpenMP threadprivate class variables across Fortran and C++
		// PGI Fortran and C++ mixed non-worksharing parallel construct needs this C++ main driver
		extern "C" void fortmain_(void);
		extern "C" void main(void);
		void main(void) { (void) fortmain_(); }
	#endif

#else
	//	(__clang__)	Naive Clang yet to support OpenMP
	// Other compilers to be tested
#endif

#endif


// C interface should avoid C++ name space mangling, thus this extern.
extern "C" {


/// PMlib C interface
/// PMlib initialization
///
/// @param[in] init_nWatch        the number of initially allocated sections.
///
/// @note   the number of sections m_nWatch is automatically increased internally. in that case,
///         the argument init_nWatch is used as incremental number.
/// @note   Unlike C++ interface, the arguments for C interface can not be ommitted.
///
void C_pm_initialize (int init_nWatch)
{

    int num_threads;
    int num_process;
    int my_rank;

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<C_pm_initialize> start debugging.\n\n\n");
	fprintf(stderr, "address of init_nWatch=%p\n", &init_nWatch);
	fprintf(stderr, "<C_pm_initialize> init_nWatch=%d\n", init_nWatch);
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
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif

	PM.setParallelMode(parallel_mode, num_threads, num_process);
	return;
}



/// PMlib C interface
/// start the measurement section
///
///   @param[in] label        the character label, i.e. name, of the measuring section
///
///
void C_pm_start (char* fc)
{
	std::string s;
	s = fc;

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<C_pm_start> fc=%s \n", s.c_str());
#endif
	if (s == "") {
		fprintf(stderr, "<C_pm_start> argument fc is empty(null)\n");
		return;
	}
	PM.start(s);
	return;
}


/// PMlib C interface
/// stop the measurement section
///
///   @param[in] label        the character label, i.e. name, of the measuring section
///
///
void C_pm_stop (char* fc)
{
	std::string s;
	s = fc;

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<C_pm_stop> fc=%s \n", s.c_str());
#endif
	if (s == "") {
		;	// section label argument is empty. stdout will be used.
	}
	PM.stop(s);
	return;
}


/// PMlib C interface
/// stop the measurement section, overload version for USER mode measurement.
///
///   @param[in] label        the character label, i.e. name, of the measuring section
///   @param[in] fpt          computing volume (FLOP) or moved data(Byte) in "USER" mode measurement
///   @param[in] tic          the number of cycles in "USER" mode measurement
///
///   @note  in "USER" mode measurement, the volume of computation or moved data is calculated
///          as fpt*tic per each start()/stop() pair.
///
void C_pm_stop_usermode (char* fc, double fpt, unsigned tic)
{
	std::string s;
	s = fc;

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<C_pm_stop_usermode> fc=%s, fpt=%8.0lf, tic=%d \n", s.c_str(), fpt, tic);
#endif
	if (s == "") {
		;	// section label argument is empty. stdout will be used.
	}
	PM.stop(s, fpt, tic);
	return;
}


/// PMlib C interface
/// generic routine to controll and output the various report of the measured statistics
///
///   @param[in] char* fc         output file name(character array). if "" , stdout is chosen.
///
void C_pm_report_top (char* fc)
{
	FILE *fp;
	std::string s;
	s=fc;
	int user_file;
	char hostname[512];

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<C_pm_report_top> fc=%s \n", s.c_str());
#endif
	if (s == "") { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_report> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}

	PM.report(fp);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib C interface
/// output BASIC report of the measured statistics
///
///   @param[in] char* fc 出力ファイル名(character文字列. ""の場合はstdoutへ出力する)
///   @param[in] char* fh ホスト名 (character文字列. ""の場合はrank 0 実行ホスト名を表示)
///   @param[in] char* fcmt 任意のコメント (character文字列)
///   @param[in] int fp_sort 測定区間の表示順(0:経過時間順にソート後表示、1:登録順で表示)
///
void C_pm_print (char* fc, char* fh, char* fcmt, int fp_sort)
{
	FILE *fp;
	std::string s, h, u;
	s=fc;
	h=fh;
	u=fcmt;
	int user_file;
	char hostname[512];

	if (s == "" ) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_print> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}

	if (h == "" ) {
		hostname[0]='\0';
		if (gethostname(hostname, sizeof(hostname)) != 0) {
			h = "unknown";
		} else {
			h = hostname;
		}
	}

	if (u == "" ) {
		u="C API";
	}

	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.print(fp, h, u, fp_sort);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib C interface
/// output DETAIL report including timing stats per MPI rank, HWPC stats per MPI rank
///
///   @param[in] char* fc 出力ファイル名(character文字列)
///   @param[in] int legend  HWPC 記号説明の表示(0:なし、1:表示する)
///   @param[in] int fp_sort 測定区間の表示順 (0:経過時間順にソート後表示、1:登録順で表示)
///
void C_pm_printdetail (char* fc, int legend, int fp_sort)
{
	FILE *fp;
	std::string s;
	s=fc;
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_printdetail> fc=%s, legend=%d, fp_sort=%d \n", s.c_str(), legend, fp_sort );
#endif

	int user_file;
	if (s == "" ) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_printdetail> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.printDetail(fp, legend, fp_sort);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib C interface
/// output DETAIL report of the designated process per thread basis
///
///   @param[in] char* fc	出力ファイル名(character文字列)
///   @param[in] int rank_ID	指定するプロセスのランク番号
///   @param[in] int fp_sort 測定区間の表示順(0:経過時間順にソート、1:登録順で表示)
///
///
void C_pm_printthreads (char* fc, int rank_ID, int fp_sort)
{
	FILE *fp;
	std::string s;
	s=fc;

	int user_file;
	if (s == "" ) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_printThreads> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.printThreads(fp, rank_ID, fp_sort);

	if (user_file == 1) {
		fclose(fp);
	}
	return;
}


/// PMlib C interface
/// output HWPC symbols and their descriptions
///
///   @param[in] char* fc	出力ファイル名(character文字列)
///
void C_pm_printlegend (char* fc)
{
	FILE *fp;
	std::string s;
	s=fc;

	int user_file;
	if (s == "" ) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_printdetail> can not open: %s\n", fc);
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


/// PMlib C interface
/// output DETAIL report per MPI rank according to the designated MPI process group
///
///   @param[in] char* fc 出力ファイル名(character文字列)
///   @param[in] MPI_Group型 p_group  MPIのgroup handle
///   @param[in] MPI_Comm型 p_comm   groupに対応するcommunicator
///   @param[in] int**型	pp_ranks groupを構成するrank番号配列へのポインタ
///   @param[in] int	group    プロセスグループ番号
///   @param[in] int	legend  HWPC 記号説明の表示(0:なし、1:表示する)
///   @param[in] int	fp_sort 測定区間の表示順
///                       (0:経過時間順にソート後表示、1:登録順で表示)
///
///   @note  HWPCを測定した計集結果があればそれも出力する
///
void C_pm_printgroup (char* fc, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group, int legend, int fp_sort)
{
	FILE *fp;
	std::string s;
	s=fc;
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_printgroup> fc=%s, group=%d, legend=%d, fp_sort=%d \n",
	//				s.c_str(), group, legend, fp_sort);
#endif

	if (s == "" ) {
		// filename is null. PMlib report is merged to stdout
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_printgroup> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.printGroup(fp, p_group, p_comm, pp_ranks, group, legend, fp_sort);

	return;
}


/// PMlib C interface
/// MPI_Comm_splitで作成するグループ毎にMPIランク詳細レポートを出力
///
///   @param[in] char*	fc	出力ファイル名(character文字列)
///   @param[in] MPI_Comm型 new_comm   groupに対応するcommunicator
///   @param[in] int	icolor   MPI_Comm_split()のカラー変数
///   @param[in] int	key      MPI_Comm_split()のkey変数
///   @param[in] int	legend  HWPC 記号説明の表示(0:なし、1:表示する)
///   @param[in] int	fp_sort 測定区間の表示順
///                       (0:経過時間順にソート後表示、1:登録順で表示)
///
void C_pm_printcomm (char* fc, MPI_Comm new_comm, int icolor, int key, int legend, int fp_sort)
{
	FILE *fp;
	std::string s;
	s=fc;
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_printcomm> fc=%s, new_comm=%d, icolor=%d, key=%d, legend=%d, fp_sort=%d \n",
	//		s.c_str(), new_comm, icolor, key, legend, fp_sort);
#endif

	if (s == "" ) {
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_printcomm> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.printComm (fp, new_comm, icolor, key, legend, fp_sort);

	return;
}


/// PMlib C interface
/// 測定途中経過の状況レポートを出力（排他測定区間を対象とする）
///
///   @param[in] fc	出力ファイル名(character文字列)
///   @param[in] comments	任意のコメント(character文字列)
///   @param[in] fp_sort 測定区間の表示順 (0:経過時間順に表示、1:登録順で表示)
///
///
void C_pm_printprogress (char* fc, char* comments, int fp_sort)
{
	FILE *fp;
	std::string s;
	s=fc;
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_printprogress> fc=%s, comments=%s, fp_sort=%d \n",
	//		s.c_str(), comments, fp_sort);
#endif

	if (s == "" ) {
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <C_pm_printcomm> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	std::string s2;
	s2=comments;
	PM.printProgress (fp, s2, fp_sort);

	return;
}


/// PMlib C interface
/// output OTF1 trace file for post processing
///
void C_pm_posttrace (void)
{
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_posttrace> \n");
#endif

	PM.postTrace ();

	return;
}


/// PMlib C interface
/// reset the measured stats of the section
///
///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
///
void C_pm_reset (char* fc)
{
	std::string s;
	s=fc;

#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_reset> fc=%s \n", s.c_str());
#endif
	if (s == "") {
		fprintf(stderr, "<C_pm_reset> argument fc is empty(null)\n");
		return;
	}
	PM.reset(s);
	return;
}


/// PMlib C interface
/// reset the measured stats of the all sections
///
///
void C_pm_resetall (void)
{
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_resetall> \n");
#endif
	PM.resetAll();
	return;
}


/// PMlib C interface
/// Set properties to the measuring section
///
///   @param[in] char* fc ラベルとなる character文字列
///   @param[in] int f_type  測定対象タイプ(0:COMM:データ移動, 1:CALC:計算)
///   @param[in] int f_exclusive 排他測定フラグ(0:false, 1:true)
///
///   @note ラベルは測定区間を識別するために用いる。
///   		各ラベル毎に対応した区間番号は内部で自動生成する
///
void C_pm_setproperties (char* fc, int f_type, int f_exclusive)
{

	std::string s;
	s=fc;
	bool exclusive;
    PerfMonitor::Type arg_type; /// 測定対象タイプ from PerfMonitor.h

#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_setproperties> fc=%s, f_type=%d, f_exclusive=%d \n", s.c_str(), f_type, f_exclusive);
#endif
	if (s == "" ) {
		fprintf(stderr, "<C_pm_setpropertie> label argument fc is (null). The call is ignored.\n");
		return;
	}
	if (f_exclusive == 1) {
		exclusive=true;
	} else if ((f_exclusive == 0)||(f_exclusive == 2)) {
		exclusive=false;
	} else {
		fprintf(stderr, "<C_pm_setpropertie> argument f_exclusive is invalid: %u . The call is ignored.\n", f_exclusive);
		return;
	}
	//	PM.setProperties(s, f_type, exclusive);
	if (f_type == 0) {
		arg_type = PM.COMM;
	} else if (f_type == 1) {
		arg_type = PM.CALC;
	} else {
		fprintf(stderr, "<C_pm_setproperties> argument f_type is invalid: %u . The call is ignored. \n", f_type);
		return;
	}
	PM.setProperties(s, arg_type, exclusive);
	return;
}


/// PMlib C interface
/// Aggregate the measured stats of all processes to the master process (rank 0)
///
void C_pm_gather (void)
{
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<C_pm_gather> \n");
#endif
	PM.gather();
	return;
}


/// PMlib C interface
/// Check if the section has been called inside of parallel region
///
///   @param[in] id         shared section number
///   @param[out] mid       class private section number
///   @param[out] inside     0/1 (0:serial region / 1:parallel region)
///
void C_pm_serial_parallel (int id, int &mid, int &inside)
{
	PM.SerialParallelRegion(id, mid, inside);
	return;
}


/// PMlib C interface
///  Stop the Root section, which means the ending of PMlib stats recording
///
void C_pm_stop_Root (void)
{
    PM.stopRoot();
    return;
}



/// PMlib C interface
///  OpenMP parallel region内のマージ処理
///  呼び出された測定区間のスレッド情報をマスタースレッドに集約する。
///
///		@param[in] id      測定区間の番号
///
///		@note  通常このAPIはPMlib内部で自動的に実行され、利用者が呼び出す必要はない。
///
void C_pm_mergethreads (int id)
{
	PM.mergeThreads(id);
	return;
}


/// PMlib C interface
/// @brief Power knob interface - Read the current value for the given power control knob
///
///   @param[in] knob  : power knob chooser 電力制御用ノブの種類
///   @param[out] value : current value for the knob  現在の値
///
/// @note the knob and its value combination must be chosen from the following table
///	@verbatim
/// knob : value : object description
///  0   : 2200, 2000 : CPU frequency in MHz
///  1   : 100, 90, 80, .. , 10 : MEMORY access throttling percentage
///  2   : 2, 4    : ISSUE instruction issue rate per cycle
///  3   : 1, 2    : PIPE number of concurrent execution pipelines 
///  4   : 0, 1, 2 : ECO mode state
///  5   : 0, 1    : RETENTION mode state DISABLED as of May 2021
///	@endverbatim
///
void C_pm_getpowerknob (int knob, int& value)
{
	PM.getPowerKnob (knob, value);
	return;
}


/// PMlib C interface
/// @brief Power knob interface - Set the new value for the given power control knob
///
///   @param[in] knob  : power knob chooser 電力制御用ノブの種類
///   @param[in] value : new value for the knob  指定する設定値
///
/// @note the knob and its value combination must be chosen from the following table
///	@verbatim
/// knob : value : object description
///  0   : 2200, 2000 : CPU frequency in MHz
///  1   : 100, 90, 80, .. , 10 : MEMORY access throttling percentage
///  2   : 2, 4    : ISSUE instruction issue rate per cycle
///  3   : 1, 2    : PIPE number of concurrent execution pipelines 
///  4   : 0, 1, 2 : ECO mode state
///  5   : 0, 1    : RETENTION mode state DISABLED as of May 2021
///	@endverbatim
///
void C_pm_setpowerknob (int knob, int value)
{
	PM.setPowerKnob (knob, value);
	return;
}


} // closing extern "C"


