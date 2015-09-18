#ifndef _PM_PERFWATCH_H_
#define _PM_PERFWATCH_H_

/* ############################################################################
 *
 * PMlib - Performance Monitor library
 *
 * Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
 * All rights reserved.
 *
 * Copyright (c) 2012-2015 Advanced Institute for Computational Science, RIKEN.
 * All rights reserved.
 *
 * ############################################################################
 */

//! @file   PerfWatch.h
//! @brief  PerfWatch class Header
//! @version rev.2.2 dated 10/30/2014 


#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstdlib>

#ifdef _OPENMP
	#include <omp.h>
#endif
#include "pmlib_papi.h"

#ifndef _WIN32
#include <sys/time.h>
#else
#include "sph_win32_util.h"   // for win32
#endif


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
    int m_typeCalc;       ///< 測定対象タイプ (0:通信, 1:計算)
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
    unsigned long  m_count_sum; ///< 「測定回数」summed over all MPI ranks
    bool m_gathered;            ///< 集計済みフラグ
    double* gather_sorted;	///< HWPC集計後ソートされた配列のポインタ
    
    /// 排他測定実行中フラグ. 非排他測定では未使用
    static bool ExclusiveStarted;
    
    /// MPI並列時の並列プロセス数と自ランク番号
    int num_process;
    int my_rank;
    
    /// 測定区間が初めてstartされる場合かどうかのフラグ
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
    int get_typeCalc(void) { return m_typeCalc; }
    
    /// 測定区間にプロパティを設定.
    ///
    ///   @param[in] label ラベル
    ///   @param[in] typeCalc  測定量のタイプ(0:通信, 1:計算)
    ///   @param[in] myID      ランク番号
    ///   @param[in] exclusive 排他測定フラグ
    ///
    void setProperties(const std::string& label, const int typeCalc, const int nPEs, const int myID, bool exclusive) {
      m_label = label;
      m_typeCalc = typeCalc;
      m_exclusive =  exclusive;
      num_process = nPEs;
      my_rank = myID;
    }
    
    /// HWPCイベントを初期化する
    ///
    void initializeHWPC(void);

    /// 測定スタート.
    ///
    void start();
    
  /// 測定区間ストップ.
  ///
  ///   @param[in] flopPerTask     測定区間の計算量(演算量Flopまたは通信量Byte)
  ///   @param[in] iterationCount  計算量の乗数（反復回数）
  ///
  ///   @note  計算量をユーザが引数で明示的に指定する場合は、そのボリュームは
  ///          １区間１回あたりでflopPerTask*iterationCount
  ///          として算出される。
  ///          引数とは関係なくHWPC内部で自動計測する場合もある。
  ///          レポート出力する情報の選択方法は以下の規則による。
  ///   @verbatim
    /**
    *
    出力レポートに表示される情報はモード・引数の組み合わせで決める
    (A) ユーザ申告モード
      - HWPC APIが利用できないシステムや環境変数HWPC_CHOOSERが指定
        されていないジョブでは自動的にユーザ申告モードで実行される。
      - ユーザ申告モードでは(1):setProperties() と(2):stop()への引数により
        出力内容が決定、HWPC詳細レポートは出力されない。
      - (1) ::setProperties(区間名, type, exclusive)の第2引数typeは
        測定量のタイプを指定する。計算(CALC)タイプか通信(COMM)タイプか
        の選択を行なう、ユーザ申告モードで有効な引数。
      - (2) ::stop (区間名, fPT, iC)の第2引数fPTは測定量。
        計算（浮動小数点演算）あるいは通信（MPI通信やメモリロードストア
        などデータ移動)の量を数値や式で与える。

        setProperties()  stop()
        type引数         fP引数     基本・詳細レポート出力
        ---------------------------------------------------------
        CALC	         指定あり   時間、fPT引数によるFlops
        COMM		     指定あり   時間、fPT引数によるByte/s
        任意             指定なし   時間のみ

    (B) HWPCによる自動算出モード
      - HWPC/PAPIが利用可能なプラットフォームで利用できる
      - 環境変数HWPC_CHOOSERの値によりユーザ申告値を用いるかPAPI情報を
        用いるかを切り替える。(FLOPS| BANDWIDTH| VECTOR| CACHE| CYCLE)

    ユーザ申告モードかHWPC自動算出モードかは、内部的に下記表の組み合わせ
    で決定される。

    環境変数     setProperties()の  stop()の
    HWPC_CHOOSER    type引数        fP引数       基本・詳細レポート出力      HWPCレポート出力
    ------------------------------------------------------------------------------------------
	NONE (無指定)   CALC            指定値       時間、fP引数によるFlops	 なし
	NONE (無指定)   COMM            指定値       時間、fP引数によるByte/s    なし
    FLOPS           無視            無視         時間、HWPC自動計測Flops     FLOPSに関連するHWPC統計情報
    VECTOR          無視            無視         時間、HWPC自動計測SIMD率    VECTORに関連するHWPC統計情報
    BANDWIDTH       無視            無視         時間、HWPC自動計測Byte/s    BANDWIDTHに関連するHWPC統計情報
    CACHE           無視            無視         時間、HWPC自動計測L1$,L2$   CACHEに関連するHWPC統計情報
     **/
  ///   @endverbatim
  ///
    void stop(double flopPerTask, unsigned iterationCount);
    
    /// 測定結果情報をランク０プロセスに集約.
    ///
    void gather(void);

    /// HWPCイベントの測定値を収集する
    ///
    void gatherHWPC(void);

    /// 測定結果の平均値・標準偏差などの基礎的な統計計算
    ///
    void statsAverage(void);

    /// 計算量としてユーザー申告値を用いるかHWPC計測値を用いるかの決定を行う
    ///
    ///   @return  戻り値とその意味合い
    ///		0: user set bandwidth
    /// 	1: user set flop counts
    /// 	2: HWPC base bandwidth
    /// 	3: HWPC base flop counts
    /// 	4: other HWPC base statistics
    int statsSwitch(void);

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
    
    /// HWPCヘッダーを出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///
    ///   @note ランク0プロセスからのみ呼び出し可能
    ///
    void printHWPCHeader(FILE* fp);
    
    /// HWPCレジェンドを出力.
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///
    ///   @note ランク0プロセスからのみ呼び出し可能
    ///
    void printHWPCLegend(FILE* fp);
    
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

    /// 単位変換.
    ///
    ///   @param[in] fops 浮動小数演算数/通信量(バイト)
    ///   @param[out] unit 単位の文字列
    ///   @param[in] typeCalc  測定対象タイプ(0:通信, 1:計算)
    ///   @return  単位変換後の数値
    ///
    static double unitFlop(double fops, std::string &unit, int typeCalc);
    
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
	void outputPapiCounterGroup (FILE* fp, MPI_Group p_group, int* pp_ranks);

  };
  
} /* namespace pm_lib */

#endif // _PM_PERFWATCH_H_

