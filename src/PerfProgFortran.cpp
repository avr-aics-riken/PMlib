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
#include "stdlib.h"
#include <unistd.h>
#include <PerfMonitor.h>

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
	//	Fujitsu traditional C++ without threadprivate class support
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


// Fortran interface should avoid C++ name space mangling, thus this extern.
extern "C" {


/// PMlib Fortran interface
/// PMlib initialization
///
/// @param[in] init_nWatch        the number of initially allocated sections.
///
/// @note   the number of sections m_nWatch is automatically increased internally. in that case,
///         the argument init_nWatch is used as incremental number.
/// @note   Unlike C++ interface, the arguments for Fortran interface can not be ommitted.
///
void f_pm_initialize_ (int& init_nWatch)
{

    int num_threads;
    int num_process;
    int my_rank;

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<f_pm_initialize_> init_nWatch=%d\n", init_nWatch);
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



/// PMlib Fortran interface
/// start the measurement section
///
///   @param[in] label        the character label, i.e. name, of the measuring section
///   @param[in] int fc_size  the length of the character label.
///
///   @note
///   	Fortran compilers automatically add an extra int argument holding the size of character.
///   @note
///   	Users do not have to explicitly give it when calling from Fortran programs.
///			for example, call f_pm_start ("myname") is good enough.
///
void f_pm_start_ (char* fc, int fc_size)
{
	std::string s=std::string(fc,fc_size);

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<f_pm_start_> fc=%s, fc_size=%d\n", s.c_str(), fc_size);
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


/// PMlib Fortran interface
/// stop the measurement section
///
///   @param[in] label        the character label, i.e. name, of the measuring section
///   @param[in] int fc_size  the length of the character label.
///
///   @note  Fortran compilers automatically add an extra fc_size argument holding the size of character.
///          Users do not have to explicitly give it when calling from Fortran programs.
///	         for example, call f_pm_stop ("myname") is good enough.
///
void f_pm_stop_ (char* fc, int fc_size)
{
	std::string s=std::string(fc,fc_size);

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<f_pm_stop_> fc=%s, fc_size=%d\n", s.c_str(), fc_size);
#endif
	if (s == "") {
		;	// section label argument is empty. stdout will be used.
	}
	PM.stop(s);
	return;
}


/// PMlib Fortran interface
/// stop the measurement section, overload version for USER mode measurement.
///
///   @param[in] label        the character label, i.e. name, of the measuring section
///   @param[in] fpt          computing volume (FLOP) or moved data(Byte) in "USER" mode measurement
///   @param[in] tic          the number of cycles in "USER" mode measurement
///   @param[in] int fc_size  the length of the character label.
///
///   @note  in "USER" mode measurement, the volume of computation or moved data is calculated
///          as fpt*tic per each start()/stop() pair.
///   @note  Fortran compilers automatically add an extra fc_size argument holding the size of character.
///          Users do not have to explicitly give it when calling from Fortran programs.
///	         for example, call f_pm_stop_usermode ("myname", fpt, tic) is good enough.
///
void f_pm_stop_usermode_ (char* fc, double& fpt, unsigned& tic, int fc_size)
{
	std::string s=std::string(fc,fc_size);

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<f_pm_stop_usermode_> fc=%s, fpt=%8.0lf, tic=%d, fc_size=%d\n", s.c_str(), fpt, tic, fc_size);
#endif
	if (s == "") {
		;	// section label argument is empty. stdout will be used.
	}
	PM.stop(s, fpt, tic);
	return;
}


/// PMlib Fortran interface
/// generic routine to controll and output the various report of the measured statistics
///
///   @param[in] char* fc         output file name(character array). if "" , stdout is chosen.
///   @param[in] int fc_size      the number of characters in fc
///
///   @note the second argument fc_size is automatically added by fortran compilers, and users
///			do not have to explicitly give it when calling from Fortran programs.
///			for example, call f_pm_print_ ("") is good enough for most use cases.
///
void f_pm_report_top_ (char* fc, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
	int user_file;
	char hostname[512];

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<f_pm_report_> fc=%s, fc_size=%d\n", s.c_str(), fc_size);
#endif
	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <f_pm_report_> can not open: %s\n", fc);
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


/// PMlib Fortran interface
/// output BASIC report of the measured statistics
///
///   @param[in] char* fc 出力ファイル名(character文字列. ""の場合はstdoutへ出力する)
///   @param[in] char* fh ホスト名 (character文字列. ""の場合はrank 0 実行ホスト名を表示)
///   @param[in] char* fcmt 任意のコメント (character文字列)
///   @param[in] int fp_sort 測定区間の表示順(0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int fc_size  fcの文字数
///   @param[in] int fh_size  fhの文字数
///   @param[in] int fcmt_size  fcmtの文字数
///
///   @note 後半の３引数fc_size, fh_size, fcmt_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///			void f_pm_print_ (char* fc, int &fp_sort, int fc_size)
///
void f_pm_print_ (char* fc, char* fh, char* fcmt, int &fp_sort, int fc_size, int fh_size, int fcmt_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
	std::string h=std::string(fh,fh_size);
	std::string u=std::string(fcmt,fcmt_size);
	int user_file;
	char hostname[512];

	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <f_pm_print_> can not open: %s\n", fc);
			fp=stdout;
			user_file=0;
		} else {
			user_file=1;
		}
	}

	if (h == "" || fh_size == 0) {
		hostname[0]='\0';
		if (gethostname(hostname, sizeof(hostname)) != 0) {
			// hostname is not available
			h = "unknown";
		} else {
			h = hostname;
		}
	}

	if (u == "" || fcmt_size == 0) {
		u="Fortran API";
	}

	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.print(fp, h, u, fp_sort);

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
///   @param[in] int fp_sort 測定区間の表示順 (0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int fc_size  出力ファイル名の文字数
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printdetail_ (char* fc, int& legend, int &fp_sort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printdetail_> fc=%s, legend=%d, fp_sort=%d, fc_size=%d \n", s.c_str(), legend, fp_sort, fc_size);
#endif

	int user_file;
	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <f_pm_printdetail_> can not open: %s\n", fc);
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


/// PMlib Fortran インタフェイス
/// 指定するプロセスのスレッド別詳細レポートを出力。
///
///   @param[in] char* fc	出力ファイル名(character文字列)
///   @param[in] int rank_ID	指定するプロセスのランク番号
///   @param[in] int fp_sort 測定区間の表示順(0:経過時間順にソート、1:登録順で表示)
///   @param[in] int fc_size  出力ファイル名の文字数
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printthreads_ (char* fc, int &rank_ID, int &fp_sort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	fprintf(stderr, "<f_pm_printThreads_> fc=%s, rank_ID=%d, fp_sort=%d, my_rank=%d, fc_size=%d \n",
		s.c_str(), rank_ID, fp_sort, my_rank, fc_size);
#endif

	int user_file;
	if (s == "" || fc_size == 0) { // if filename is null, report to stdout
		fp=stdout;
		user_file=0;
	} else {
		fp=fopen(fc,"a");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <f_pm_printThreads_> can not open: %s\n", fc);
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
///   @param[in] int	fp_sort 測定区間の表示順
///                       (0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int	fc_size  character文字列ラベルの長さ（文字数）
///
///   @note  MPI_Group, MPI_Comm型は呼び出すFortran側では integer 型である
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///   @note  HWPCを測定した計集結果があればそれも出力する
///
void f_pm_printgroup_ (char* fc, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int& group, int& legend, int &fp_sort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printgroup_> fc=%s, group=%d, legend=%d, fp_sort=%d, fc_size=%d \n",
	//				s.c_str(), group, legend, fp_sort, fc_size);
#endif

	if (s == "" || fc_size == 0) {
		// filename is null. PMlib report is merged to stdout
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <f_pm_printgroup_> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.printGroup(fp, p_group, p_comm, pp_ranks, group, legend, fp_sort);

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
///   @param[in] int	fp_sort 測定区間の表示順
///                       (0:経過時間順にソート後表示、1:登録順で表示)
///   @param[in] int	fc_size  character文字列ラベルの長さ（文字数）
///
///   @note  MPI_Group, MPI_Comm型は呼び出すFortran側では integer 型である
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printcomm_ (char* fc, MPI_Comm new_comm, int& icolor, int& key, int& legend, int& fp_sort, int fc_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printcomm_> fc=%s, new_comm=%d, icolor=%d, key=%d, legend=%d, fp_sort=%d, fc_size=%d \n",
	//		s.c_str(), new_comm, icolor, key, legend, fp_sort, fc_size);
#endif

	if (s == "" || fc_size == 0) {
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <f_pm_printcomm_> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	PM.printComm (fp, new_comm, icolor, key, legend, fp_sort);

	return;
}


/// PMlib Fortran インタフェイス
/// 測定途中経過の状況レポートを出力（排他測定区間を対象とする）
///
///   @param[in] fc	出力ファイル名(character文字列)
///   @param[in] comments	任意のコメント(character文字列)
///   @param[in] fp_sort 測定区間の表示順 (0:経過時間順に表示、1:登録順で表示)
///   @param[in] fc_size  fc文字列の長さ（文字数）
///   @param[in] comments_size  comments文字列の長さ（文字数）
///
///   @note fc_sizeはFortranコンパイラが自動的に追加してしまう引数。
///			ユーザがFortranプログラムから呼び出す場合に指定する必要はない。
///
void f_pm_printprogress_ (char* fc, char* comments, int& fp_sort, int fc_size, int comments_size)
{
	FILE *fp;
	std::string s=std::string(fc,fc_size);
#ifdef DEBUG_PRINT_MONITOR
	//	fprintf(stderr, "<f_pm_printprogress_> fc=%s, comments=%s, fp_sort=%d, fc_size=%d, comments_size=%d \n",
	//		s.c_str(), comments, fp_sort, fc_size, comments_size);
#endif

	if (s == "" || fc_size == 0) {
		fp=stdout;
	} else {
		fp=fopen(fc,"w+");
		if (fp == NULL) {
			//	fprintf(stderr, "*** warning <f_pm_printcomm_> can not open: %s\n", fc);
			fp=stdout;
		}
	}
	if (fp_sort != 0 && fp_sort != 1) fp_sort = 0;

	std::string s2=std::string(comments,comments_size);
	PM.printProgress (fp, s2, fp_sort);

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
/// 測定区間にプロパティを設定.
///
///   @param[in] char* fc ラベルとなる character文字列
///   @param[in] int f_type  測定対象タイプ(0:COMM:データ移動, 1:CALC:計算)
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
		fprintf(stderr, "<f_pm_setproperties> label argument fc is (null). The call is ignored.\n");
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


/// PMlib Fortran interface
///  Count the number of shared sections
///
///   @param[out] nSections		 	number of shared sections
///
void f_pm_sections_ (int &nSections)
{

	PM.countSections(nSections);

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<f_pm_sections_> nSections=%d \n", nSections);
#endif
	return;
}


/// PMlib Fortran interface
///  Check if the section has been called inside of parallel region
///
///   @param[in] id		 	shared section number
///   @param[out] mid		class private section number
///   @param[out] inside	 0/1 (0:serial region / 1:parallel region)
///
///
void f_pm_serial_parallel_ (int &id, int &mid, int &inside)
{

	PM.SerialParallelRegion(id, mid, inside);

#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<f_pm_serial_parallel_> id=%d mid=%d inside=%d \n", id, mid, inside);
#endif
	return;
}


/// PMlib Fortran interface
///  Stop the Root section, which means the ending of PMlib stats recording
///
void f_pm_stop_root_ (void)
{
	PM.stopRoot();
	return;
}


/// PMlib Fortran interface
///  OpenMP parallel region内のマージ処理
///  呼び出された測定区間のスレッド情報をマスタースレッドに集約する。
/// 
///   @param[in] id		 	shared section number
/// 
///   @note  通常このAPIはPMlib内部で自動的に実行され、利用者が呼び出す必要はない。
///   @note この測定区間の番号はスレッドプライベートな番号ではなく、共通番号であることに注意。
///
void f_pm_mergethreads_ (int &id)
{
	PM.mergeThreads(id);
	return;
}


/// PMlib Fortran interface
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
void f_pm_getpowerknob_ (int& knob, int& value)
{
	PM.getPowerKnob (knob, value);
	return;
}


/// PMlib Fortran interface
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
void f_pm_setpowerknob_ (int& knob, int& value)
{
	PM.setPowerKnob (knob, value);
	return;
}


} // closing extern "C" // PMlib Fortran インタフェイス終了


