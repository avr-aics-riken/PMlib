#ifndef _PM_PERFWATCH_H_
#define _PM_PERFWATCH_H_

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

//! @file   PerfWatch.h
//! @brief  PerfWatch class Header
//! @version rev.5.8


#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstdlib>
#include <string.h>

#ifdef _OPENMP
	#include <omp.h>
#endif

#include "pmlib_papi.h"
#include "pmlib_power.h"
#include "pmlib_otf.h"

#ifndef _WIN32
#include <sys/time.h>
#else
#include "sph_win32_util.h"   // for Windows win32 GetSystemTimeAsFileTime API?
#endif

namespace pm_lib {

  /// デバッグ用マクロ
#define PM_Exit(x) \
((void)fprintf(stderr, "*** continue from <%s> line:%u\n", __FILE__, __LINE__))
//	Do not exit when an error occurs.
//	((void)printf("exit at %s:%u\n", __FILE__, __LINE__), exit((x)))



  /**
   * 計算性能「測定時計」クラス.
   */
  class PerfWatch {
  public:

    // プロパティ
    std::string m_label;   ///< 測定区間のラベル
    int m_id;             ///< 測定区間のラベルに対応する番号
    int m_typeCalc;        ///< 測定対象タイプ (0:通信, 1:計算)
    bool m_exclusive;      ///< 測定区間の排他性フラグ (false, true)
    bool m_in_parallel;    /// 測定区間が並列領域の内部にあるか(false, true)

    /// MPI並列時の並列プロセス数と自ランク番号
    int num_process;
    int my_rank;

    // 測定値の積算量
    long m_count;          ///< 測定回数 (プロセス内の全スレッドの最大値)
    double m_time;         ///< 時間(秒)
    double m_flop;         ///< 浮動小数点演算量or通信量(バイト)
    double m_percentage;   ///< Percentage of vectorization or cache hit

    // 統計量(全プロセスに関する統計量でランク0のみが保持する)
    long m_count_sum;    ///< 測定回数 (全プロセスの合計値)
    long m_count_av;     ///< 測定回数の平均値
    double m_time_av;    ///< 時間の平均値
    double m_time_sd;    ///< 時間の標準偏差
    double m_flop_av;    ///< 浮動小数点演算量or通信量の平均値
    double m_flop_sd;    ///< 浮動小数点演算量or通信量の標準偏差
    double m_time_comm;  ///< 通信部分の最大値

    int level_POWER;	///< 電力情報レベル 0(no), 1(NODE), 2(NUMA), 3(PARTS)
    double m_power_av;    ///< average value of power consumption meter reading
    int level_OTF;	     ///< OTF tracing 出力レベル 0(no), 1(yes), 2(full)
    std::string otf_filename;    ///< OTF filename headings
                        //	master 		: otf_filename + .otf
                        //	definition	: otf_filename + .mdID + .def
                        //	event		: otf_filename + .mdID + .events
                        //	marker		: otf_filename + .mdID + .marker

	struct pmlib_papi_chooser my_papi;

	struct pmlib_power_chooser my_power;

  private:
    /// OpenMP並列時のスレッド数と自スレッド番号
    int num_threads;
    int my_thread;

    // 測定時の補助変数
    double m_startTime;  ///< 測定区間の測定開始時刻
    double m_stopTime;   ///< 測定区間の測定終了時刻

    // 測定値集計時の補助変数
    double* m_timeArray;         ///< 「時間」集計用配列
    double* m_flopArray;         ///< 「浮動小数点演算量or通信量」集計用配列
    long* m_countArray; ///< 「測定回数」集計用配列
    double* m_sortedArrayHWPC;   ///< 集計後ソートされたHWPC配列のポインタ

    /// 測定区間に関する各種の判定フラグ ：  bool値(true|false)
    bool m_is_set;         /// 測定区間がプロパティ設定済みかどうか
    bool m_is_healthy;     /// 測定区間に排他性・非排他性の矛盾がないか
    bool m_threads_merged; /// 全スレッドの情報をマスタースレッドに集約済みか
    bool m_gathered;       /// 全プロセスの結果をランク0に集計済みかどうか
    bool m_started;        /// 測定区間がstart済みかどうか
		/// Remark m_started is usefule for serial construct only.
		///        in parallel construct


  public:
    /// コンストラクタ.
    PerfWatch() : m_time(0.0), m_flop(0.0), m_count(0), m_started(false),
      my_rank(-1), m_timeArray(0), m_flopArray(0), m_countArray(0),
      m_sortedArrayHWPC(0), m_is_set(false), m_is_healthy(true),
      m_in_parallel(false) {
	#ifdef DEBUG_PRINT_WATCH
		int i_thread_constractor;
		#ifdef _OPENMP
		i_thread_constractor = omp_get_thread_num();
		#else
		i_thread_constractor = 0;
		#endif
		fprintf(stderr, "\t<PerfWatch> constructor : thread=%d, &m_id=%p \n",
			i_thread_constractor,  &m_id);
	#endif
	}

/***
    /// We should let the default destructor handle the clean up
    ~PerfWatch() {
      if (m_timeArray != NULL)  delete[] m_timeArray;
      if (m_flopArray != NULL)  delete[] m_flopArray;
      if (m_countArray != NULL) delete[] m_countArray;
      if (m_sortedArrayHWPC != NULL) delete[] m_sortedArrayHWPC;
		#ifdef DEBUG_PRINT_WATCH
		//	if (my_rank == 0) {
    	fprintf(stderr, "\t <PerfWatch> destructor rank %d thread %d for [%s]\n", my_rank, my_thread, m_label.c_str() );
		//	}
		#endif
    }
 ***/

    /// 測定モードを返す
    int get_typeCalc(void) { return m_typeCalc; }

    /// 測定区間にプロパティを設定.
    ///
    ///   @param[in] label     ラベル
    ///   @param[in] id       ラベルに対応する番号
    ///   @param[in] typeCalc  測定量のタイプ(0:通信, 1:計算)
    ///   @param[in] nPEs      プロセス数
    ///   @param[in] my_rank   自ランク番号
    ///   @param[in] exclusive 排他測定フラグ
    ///
    void setProperties(const std::string label, int id, int typeCalc, int nPEs, int my_rank, int num_threads, bool exclusive);

    /// HWPCイベントを初期化する
    ///
    void initializeHWPC(void);

    /// initialize Power API related variables
    ///
    void initializePowerWatch(int num);


    /// OTF 用の初期化
    ///
    void initializeOTF(void);

    /// 測定スタート.
    ///
    void start();

    /// 測定区間ストップ.
    ///
    ///   @param[in] flopPerTask     測定区間の計算量(演算量Flopまたは通信量Byte)
    ///   @param[in] iterationCount  計算量の乗数（反復回数）
    ///
    ///   @note  引数はユーザ申告モードの場合にのみ利用され、計算量を
    ///          １区間１回あたりでflopPerTask*iterationCount として算出する。\n
    ///          HWPCによる自動算出モードでは引数は無視され、
    ///          内部で自動計測するHWPC統計情報から計算量を決定決定する。\n
    ///          レポート出力する情報の選択方法はPerfMonitor::stop()の規則による。\n
    ///
    void stop(double flopPerTask, unsigned iterationCount);

    /// 測定のリセット
    ///
    void reset(void);

    /// 測定結果情報をランク０プロセスに集約.
    ///
    void gather(void);

    /// HWPCにより測定したプロセスレベルのイベントカウンター測定値を収集する
    ///
    void gatherHWPC(void);

    /// HWPCにより測定したスレッドレベルのイベントカウンター測定値を収集する
    ///
    void gatherThreadHWPC(void);

	/// start measuring the power of the section
	///
	///   @param[in] PWR_Cntxt pacntxt
	///   @param[in] PWR_Cntxt extcntxt
	///   @param[in] PWR_Obj obj_array
	///   @param[in] PWR_Obj obj_ext
	///
	///   @note the arguments are Power API objects and attributes
	///
	void power_start(PWR_Cntxt pacntxt, PWR_Cntxt extcntxt, PWR_Obj obj_array[], PWR_Obj obj_ext[]);
	
	/// stop measuring the power of the section
	///
	///   @param[in] PWR_Cntxt pacntxt
	///   @param[in] PWR_Cntxt extcntxt
	///   @param[in] PWR_Obj obj_array
	///   @param[in] PWR_Obj obj_ext
	///
	///   @note the arguments are Power API objects and attributes
	///
	void power_stop(PWR_Cntxt pacntxt, PWR_Cntxt extcntxt, PWR_Obj obj_array[], PWR_Obj obj_ext[]);

    /// gather the estimated power consumption of all processes
    ///
    void gatherPOWER(void);

    /// 測定結果の平均値・標準偏差などの基礎的な統計計算
    ///
    void statsAverage(void);

    /// 計算量の選択を行う
    ///
    /// @return  戻り値とその意味合い \n
    ///   0: ユーザが引数で指定したデータ移動量(バイト)を採用する \n
    ///   1: ユーザが引数で指定した演算量(浮動小数点演算量)を採用する \n
    ///   2: HWPC が自動的に測定する memory read event \n
    ///   3: HWPC が自動的に測定する flops event \n
    ///   4: HWPC が自動的に測定する vectorization (SSE, AVX, etc) event \n
    ///   5: HWPC が自動的に測定する cache hit, miss \n
    ///   6: HWPC が自動的に測定する cycles, instructions \n
    ///   7: HWPC が自動的に測定する memory write (writeback, streaming store)\n
    ///
    /// @note
    ///   計算量としてユーザー申告値を用いるかHWPC計測値を用いるかの決定を行う
    ///   もし環境変数HWPC_CHOOSERの値が指定されている場合はそれが優先される
    ///
    int statsSwitch(void);

    /// 計算量の単位変換.
    ///
    ///   @param[in] fops 浮動小数演算数/通信量(バイト)
    ///   @param[out] unit 単位の文字列
    ///   @param[in] is_unit ユーザー申告値かHWPC自動測定値かの指定 \n
    ///   	\t\t is_unitは通常PerfWatch::statsSwitch()で事前に決定する\n
    ///
    ///   @return  単位変換後の数値
    ///
    static double unitFlop(double fops, std::string &unit, int is_unit);

    /// 測定区間のラベル情報をOTF に出力
    ///   @param[in] label     ラベル
    ///   @param[in] id       ラベルに対応する番号
    ///
    void labelOTF(const std::string& label, int id);

    /// OTF 出力処理を終了する
    ///
    void finalizeOTF(void);

    /// MPIランク別測定結果を出力. 非排他測定区間も出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] totalTime 全排他測定区間での計算時間(平均値)の合計
    ///
    ///   @note ランク0プロセスからのみ呼び出し可能
    ///
    void printDetailRanks(FILE* fp, double totalTime);

    ///   Groupに含まれるMPIランク別測定結果を出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] totalTime 全排他測定区間での計算時間(平均値)の合計
    ///   @param[in] p_group プロセスグループ番号。0の時は全プロセスを対象とする
    ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
    ///
    ///   @note ランク0プロセスからのみ呼び出し可能
    ///
    void printGroupRanks(FILE* fp, double totalTime, MPI_Group p_group, int* pp_ranks);

    /// Show the PMlib related environment variables
    ///
    ///   @param[in] fp report file pointer
    ///
    void printEnvVars(FILE* fp);

    /// HWPCレジェンドを出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///
    void printHWPCLegend(FILE* fp);

    /// 指定するランクのスレッド別詳細レポートを出力。
    ///
    ///   @param[in] fp           出力ファイルポインタ
    ///   @param[in] rank_ID      出力対象ランク番号を指定する
    ///
    void printDetailThreads(FILE* fp, int rank_ID);

    /// Show the header line for the averaged HWPC statistics in the Basic report
    ///
    ///   @param[in] fp         report file pointer
    ///   @param[in] maxLabelLen    maximum label field string length
    ///
    void printBasicHWPCHeader(FILE* fp, int maxLabelLen);

    /// Report the averaged HWPC statistics as the Basic report
    ///
    ///   @param[in] fp         report file pointer
    ///   @param[in] maxLabelLen    maximum label field string length
    ///
    ///     @note   remark that power consumption is reported per node, not per process
    ///
    void printBasicHWPCsums(FILE* fp, int maxLabelLen);

    /// HWPCイベントの測定結果と統計値を出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] s_label 区間のラベル
    ///
    ///   @note ランク0プロセスからのみ呼び出し可能
    ///
    void printDetailHWPCsums(FILE* fp, std::string s_label);

    ///   Groupに含まれるMPIプロセスのHWPC測定結果を区間毎に出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] s_label 区間のラベル
    ///   @param[in] p_group プロセスグループ番号。0の時は全プロセスを対象とする
    ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
    ///
    ///   @note ランク0プロセスからのみ呼び出し可能
    ///
    void printGroupHWPCsums(FILE* fp, std::string s_label, MPI_Group p_group, int* pp_ranks);

    /// 時刻を取得
    ///
    ///   @return 時刻値(秒)
    ///
    ///   @note 京コンピュータ、FX100、Intel Xeonでは専用の高精度タイマー
    ///         を呼び出す。
    ///         一般のUnix/Linuxではgettimeofdayシステムコールを呼び出す。
    ///
    double getTime();

    ///   CPU動作周波数を読み出して内部保存する。
    ///
    void read_cpu_clock_freq();

    ///	copy in HWPC values from master thread to shared "papi" struct
    ///
    void mergeMasterThread(void);

    ///	merge private HWPC values from the parallel threads to the master
    ///
    void mergeParallelThread(void);

    ///	re-calculate the aggregate HWPC values from all threads
    ///
    void updateMergedThread(void);

    ///
    void selectPerfSingleThread(int i_thread);

  private:
    /// エラーメッセージ出力.
    ///
    ///   @param[in] func メソッド名
    ///   @param[in] fmt  出力フォーマット文字列
    ///
    void printError(const char* func, const char* fmt, ...);

    ///	start() calls following internal functions
    void startSectionSerial();
    void startSectionParallel();
    ///	stop() calls following internal functions
    void stopSectionSerial(double flopPerTask, unsigned iterationCount);
    void stopSectionParallel(double flopPerTask, unsigned iterationCount);

	/// HWPC related internal functions
	void identifyARMplatform (void);
	void createPapiCounterList (void);
	void sortPapiCounterList (void);
	void outputPapiCounterHeader (FILE* fp, std::string s_label);
	void outputPapiCounterList (FILE* fp);
	void outputPapiCounterLegend (FILE* fp);
	void outputPapiCounterGroup (FILE* fp, MPI_Group p_group, int* pp_ranks);

	/// Power API related internal functions
	int my_power_bind_start(PWR_Cntxt pacntxt, PWR_Cntxt extcntxt,
		PWR_Obj obj_array[], PWR_Obj obj_ext[],
		uint64_t pa64timer[], double w_joule[]);
	int my_power_bind_stop (PWR_Cntxt pacntxt, PWR_Cntxt extcntxt,
		PWR_Obj obj_array[], PWR_Obj obj_ext[],
		uint64_t pa64timer[], double w_joule[]);


  };	// end of class PerfWatch

} /* namespace pm_lib */

#endif // _PM_PERFWATCH_H_

