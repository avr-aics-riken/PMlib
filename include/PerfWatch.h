#ifndef _PM_PERFWATCH_H_
#define _PM_PERFWATCH_H_

/* ############################################################################
 *
 * PMlib - Performance Monitor library
 *
 * Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
 * All rights reserved.
 *
 * Copyright (c) 2012-2014 Advanced Institute for Computational Science, RIKEN.
 * All rights reserved.
 *
 * ############################################################################
 */

//! @file   PerfWatch.h
//! @brief  PerfWatch class Header
//! @version rev.2.2 dated 10/30/2014 

#ifdef _PM_WITHOUT_MPI_
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstdlib>

#ifndef _WIN32
#include <sys/time.h>
#else
#include "sph_win32_util.h"   // for win32
#endif

#include "pmlib_papi.h"

namespace pm_lib {

  /// デバッグ用マクロ
#define PM_Exit(x) \
((void)printf("exit at %s:%u\n", __FILE__, __LINE__), exit((x)))
  
  
  /**
   * 計算性能「測定時計」クラス.
   */
  class PerfWatch {
  public:
    
    // プロパティ
    std::string m_label;   ///< ラベル
    int m_typeCalc;       ///< 測定対象タイプ (0:通信, 1:計算, 2:自動決定)
    bool m_exclusive;      ///< 排他測定フラグ
    
    // 測定値の積算量
    double m_time;         ///< 時間(秒)
    double m_flop;         ///< 浮動小数点演算量or通信量(バイト)
    unsigned long m_count; ///< 測定回数
    
    // 統計量
    double m_time_av;    ///< 時間の平均値(ランク0のみ)
    double m_time_sd;    ///< 時間の標準偏差(ランク0のみ)
    double m_flop_av;    ///< 浮動小数点演算量or通信量の平均値(ランク0のみ)
    double m_flop_sd;    ///< 浮動小数点演算量or通信量の標準偏差(ランク0のみ)
    double m_time_comm;  ///< 通信部分の最大値（ランク0のみ）
    
    bool m_valid;        ///< 測定回数が全ランクで等しいかどうかのフラグ(ランク0のみ)
    
	struct pmlib_papi_chooser my_papi;

  private:
    // 測定時の補助変数
    double m_startTime;  ///< 測定開始時刻
    bool m_started;      ///< 測定中フラグ
    
    // 測定値集計時の補助変数
    double* m_timeArray;         ///< 「時間」集計用配列
    double* m_flopArray;         ///< 「浮動小数点演算量or通信量」集計用配列
    unsigned long* m_countArray; ///< 「測定回数」集計用配列
    bool m_gathered;             ///< 集計済みフラグ
    
    /// 排他測定実行中フラグ. 非排他測定では未使用
    static bool ExclusiveStarted;
    
    /// 並列時の自ランク番号
    int my_rank;
    
    ///
    bool m_is_first;      /// true if the first instance

  public:
    /// コンストラクタ.
    PerfWatch() : m_time(0.0), m_flop(0.0), m_count(0), m_started(false), 
    m_gathered(false), m_valid(true), my_rank(0),
    m_timeArray(0), m_flopArray(0), m_countArray(0), m_is_first(true) {}
    
    /// デストラクタ.
    ~PerfWatch() {
      if (m_timeArray)  delete[] m_timeArray;
      if (m_flopArray)  delete[] m_flopArray;
      if (m_countArray) delete[] m_countArray;
    }
    
    /// 測定モードを返す
    int get_typeCalc(void) { return m_typeCalc; }	// rev.2.2 type changed
    
    /// 測定時計にプロパティを設定.
    ///
    ///   @param[in] label ラベル
    ///   @param[in] typeCalc  測定対象タイプ(0:通信, 1:計算, 2:自動決定)
    ///   @param[in] myID      ランク番号
    ///   @param[in] exclusive 排他測定フラグ
    ///
    void setProperties(const std::string& label, const int typeCalc, const int myID, bool exclusive) {
      m_label = label;
      m_typeCalc = typeCalc;
      m_exclusive =  exclusive;
      my_rank = myID;
    }
    
    /// 測定スタート.
    ///
    void start();
    
    /// 測定ストップ.
    ///
    ///   @param[in] flopPerTask 「タスク」あたりの計算量/通信量(バイト)
    ///   @param[in] iterationCount  実行「タスク」数
    ///
    void stop(double flopPerTask, unsigned iterationCount);
    
    /// 測定結果情報をランク０に集約.
    ///
    void gather();

    /// MPIランク別測定結果を出力. 非排他測定区間も出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] totalTime 全排他測定区間での計算時間(平均値)の合計
    ///
    ///   @note ランク0からのみ, 呼び出し可能
    ///
    void printDetailRanks(FILE* fp, double totalTime);
    
    /// HWPCイベントを初期化する
    ///
    void initializeHWPC(void);

    /// HWPCイベントの測定値を収集する
    ///
    void gatherHWPC(void);
    
    /// HWPCヘッダーを出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///
    void printHWPCHeader(FILE* fp);
    /// HWPCレジェンドを出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///
    void printHWPCLegend(FILE* fp);
    /// HWPCイベントの測定結果と統計値を出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///
    void printDetailHWPCsums(FILE* fp, std::string s_label);
    
    /// 単位変換.
    ///
    ///   @param[in] fops 浮動小数演算数/通信量(バイト)
    ///   @param[out] unit 単位の文字列
    ///   @param[in] typeCalc  測定対象タイプ(0:通信, 1:計算, 2:自動決定)
    ///   @return  単位変換後の数値
    ///
    static double flops(double fops, std::string &unit, int typeCalc);
    
  private:
    /// 時刻を取得.
    ///
    ///   Unix/Linux: gettimeofdayシステムコールを使用.
    ///   Windows: GetSystemTimeAsFileTime API(sph_win32_util.h)を使用.
    ///
    ///   @return 時刻値(秒)
    ///
    double getTime();
    
    /// エラーメッセージ出力.
    ///
    ///   @param[in] func メソッド名
    ///   @param[in] fmt  出力フォーマット文字列
    ///
    void printError(const char* func, const char* fmt, ...);

  private:
	void createPapiCounterList (void);
	void sortPapiCounterList (void);
	void outputPapiCounterHeader (FILE* fp, std::string s_label);
	void outputPapiCounterList (FILE* fp);
	void outputPapiCounterLegend (FILE* fp);
	double countPapiFlop (pmlib_papi_chooser my_papi);
	double countPapiByte (pmlib_papi_chooser my_papi);

  };
  
} /* namespace pm_lib */

#endif // _PM_PERFWATCH_H_

