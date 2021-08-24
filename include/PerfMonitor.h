#ifndef _PM_PERFMONITOR_H_
#define _PM_PERFMONITOR_H_

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

//! @file   PerfMonitor.h
//! @brief  PerfMonitor class Header
#include <cstdio>
#include <cstdlib>
#include <map>
#include <list>

#ifdef DISABLE_MPI
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif

#include "PerfWatch.h"
#include "pmVersion.h"

namespace pm_lib {

  /**
   * PerfMonitor クラス 計算性能測定を行うクラス関数と変数
   */
  class PerfMonitor {

   //< General comments regarding PMlib C++ interface functions
   /// @name PMlib C++ API
   ///
   /// @note  In most cases, users will have to call only 4 types of PMlib APIs
   ///  - intialize (nWatch)
   ///  - start (label)
   ///  - stop (label)
   ///  - report (file)
   ///
   /// @note Other routines are optional. Other routines listed in this document 
   ///     can be called in combination with the above 4 routines.
   ///     Such usage is for advanced users.
   ///
   /// @note All the functions are in PerfMonitor class. However, in the caes of
   ///		C++ user application which defines PMlib start/stop sections inside of
   ///		OpenMP parallel region should call PerfReport call report().
   ///		See doc/src_advanced/parallel_thread.cpp
   ///

  public:

    enum Type {
      COMM,  ///< 測定量のタイプenumerate：データ移動
      CALC,  ///< 測定量のタイプenumerate：演算
    };

    int num_process;           ///< 並列プロセス数
    int num_threads;           ///< 並列スレッド数
    int my_rank;               ///< 自ランク番号
    int my_thread;             ///< thread number
    int m_nWatch;              ///< 測定区間数
    int init_nWatch;           ///< 初期に確保する測定区間数
    int reserved_nWatch;       ///< リザーブ済みの測定区間数

    int level_POWER;   ///< 電力制御のレベル 0(no), 1(NODE), 2(NUMA), 3(PARTS)
    int num_power;     ///< 電力測定オブジェクトの総数
	// type of PWR_Cntxt and PWR_Obj is defined in include/pmlib_power.h
	PWR_Cntxt pm_pacntxt ;
	PWR_Cntxt pm_extcntxt ;
	PWR_Obj   pm_obj_array[Max_power_stats];
	PWR_Obj   pm_obj_ext[Max_power_stats];

  private:
    bool is_PMlib_enabled;     /*!< PMlibの動作を有効にするフラグ<br>
    	//	@note 環境変数BYPASS_PMLIBを定義（任意値）してアプリを実行すると
		//	PMlibを無効化した動作となり、性能統計処理を行わない */
    bool is_MPI_enabled;       ///< PMlibの対応動作可能フラグ:MPI
    bool is_OpenMP_enabled;	   ///< PMlibの対応動作可能フラグ:OpenMP
    bool is_PAPI_enabled;      ///< PMlibの対応動作可能フラグ:PAPI
    bool is_POWER_enabled;     ///< PMlibの対応動作可能フラグ:Power API
    bool is_OTF_enabled;       ///< PMlibの対応動作可能フラグ:OTF tracing 出力
    bool is_Root_active;       ///< 背景区間(Root区間)の動作フラグ
    bool is_exclusive_construct; ///< 測定区間の重なり状態検出フラグ

    std::string parallel_mode; /*!< 並列動作モード
      // {Serial| OpenMP| FlatMPI| Hybrid} */
    std::string env_str_hwpc;  /*!< 環境変数 HWPC_CHOOSERの値
      // {FLOPS| BANDWIDTH| VECTOR| CACHE| CYCLE| LOADSTORE| USER} */
    std::string env_str_report;  /*!< 環境変数 PMLIB_REPORTの値
      // {BASIC| DETAIL| FULL} */

    PerfWatch* m_watchArray;   /*!< 測定区間の配列
      // @note PerfWatchのインスタンスは全部で m_nWatch 生成される。<br>
      // m_watchArray[0] :PMlibが定義するRoot区間、<br>
      // m_watchArray[1 .. m_nWatch] :ユーザーが定義する各区間 */

    unsigned* m_order;         ///< 測定区間ソート用のリスト m_order[m_nWatch]

    std::map<std::string, int > m_map_sections; /// map of section name and ID


  public:
    /// コンストラクタ.
    PerfMonitor() : my_rank(-1), m_watchArray(0) {
		#ifdef DEBUG_PRINT_MONITOR
		//	if (my_rank == 0) {
		fprintf(stderr, "<PerfMonitor> constructor \n");
		// }
		#endif
	}

/***
    /// We should let the default destructor handle the clean up
    ~PerfMonitor() {
		if (m_watchArray) delete[] m_watchArray;
		if (m_order) delete[] m_order;
		#ifdef DEBUG_PRINT_MONITOR
		//	if (my_rank == 0) {
		fprintf(stderr, "<PerfMonitor> destructor rank %d thread %d deleted %d sections\n", my_rank, my_thread, m_nWatch);
		// }
		#endif
	}
 ***/


    /// PMlibの内部初期化
    ///
    /// 測定区間数分の内部領域を確保しする。並列動作モード、サポートオプション
    /// の認識を行い、実行時のオプションによりHWPC、OTF出力用の初期化も行う。
    ///
    /// @param[in] init_nWatch 最初に確保する測定区間数（C++では省略可能）
    ///
    /// @note
    /// 測定区間数分の内部領域を最初にinit_nWatch区間分を確保する。
    /// 測定区間数が不足したらその都度動的にinit_nWatch追加する。
    ///
    void initialize (int init_nWatch=100);


    /// 測定区間とそのプロパティを設定.
    ///
    ///   @param[in] label 測定区間に与える名前の文字列
    ///   @param[in] type  測定計算量のタイプ(COMM:データ移動, CALC:演算)
    ///   @param[in] exclusive 排他測定フラグ。bool型(省略時true)、
    ///                    FortranおよびCでの引数仕様は整数型(0:false, 1:true)
    ///
    ///   @note labelラベル文字列は測定区間を識別するために用いる。
    ///   各ラベル毎に対応した区間番号を内部で自動生成する
    ///   最初に確保した区間数init_nWatchが不足したら動的にinit_nWatch追加する
    ///   第１引数は必須。第２引数は明示的な自己申告モードの場合に必須。
    ///   第３引数は省略可
    ///
    void setProperties(const std::string& label, Type type=CALC, bool exclusive=true);


    /// Read the current value for the given power control knob
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
    void getPowerKnob(int knob, int & value);


    /// Set the new value for the given power control knob
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
    void setPowerKnob(int knob, int value);


    /// 測定区間スタート
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///
    ///   @note a section can be called either from serial construct or from
	///		parallel construc. But a section can not be called from 
	///		both of serial construc and parallel construct.
    ///
    void start (const std::string& label);


    /// 測定区間ストップ
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///   @param[in] flopPerTask 測定区間の計算量(演算量Flopまたは通信量Byte) :省略値0
    ///   @param[in] iterationCount  計算量の乗数（反復回数）:省略値1
    ///
    ///   @note  第２、第３引数はユーザ申告モードの場合にのみ利用される。 \n
    ///   @note  測定区間の計算量は次のように算出される。 \n
    ///          (A) ユーザ申告モードの場合は １区間１回あたりで flopPerTask*iterationCount \n
    ///          (B) HWPCによる自動算出モードの場合は引数とは関係なくHWPC内部値を利用\n
    ///   @note  HWPC APIが利用できないシステムや環境変数HWPC_CHOOSERが指定
    ///          されていないジョブでは自動的にユーザ申告モードで実行される。\n
    ///   @note  出力レポートに表示される計算量は測定のモード・引数の
    ///          組み合わせで以下の規則により決定される。 \n
    ///   @verbatim
    /**
    (A) ユーザ申告モード
      - ユーザ申告モードでは(1)setProperties() と(2)stop()への引数により出力内容が決定される。
        (1) setProperties(区間名, type, exclusive)の第2引数typeが計算量のタイプを指定する。
        (2) stop (区間名, fPT, iC)の第2引数fPTは計算量（浮動小数点演算、データ移動)を指定する。
      - ユーザ申告モードで 計算量の引数が省略された場合は時間のみレポート出力する。
    (B) HWPCによる自動算出モード
      - HWPC/PAPIが利用可能なプラットフォームで利用できる
      - 環境変数HWPC_CHOOSERの値により測定情報を選択する。(FLOPS| BANDWIDTH| VECTOR| CACHE| CYCLE| LOADSTORE)
        環境変数HWPC_CHOOSERが指定された場合（USER以外の値を指定した場合）は自動的にHWPCが利用される。
     **/
    ///   @endverbatim
    ///
    void stop(const std::string& label, double flopPerTask=0.0, unsigned iterationCount=1);


    /// 測定区間のリセット
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///
    void reset (const std::string& label);


    /// 全測定区間のリセット
    ///
    ///
    void resetAll (void);


    /// 全プロセスの測定結果、全スレッドの測定結果を集約
    /// 利用者は通常このAPIを呼び出す必要はない。
    ///
    /// @note  以下の処理を行う。
    ///       各測定区間の全プロセスの測定結果情報をマスタープロセスに集約。
    ///       測定結果の平均値・標準偏差などの基礎的な統計計算。
    ///       経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
    ///       各測定区間のHWPCイベントの統計値を取得する。
    ///       各プロセスの全スレッド測定結果をマスタースレッドに集約
    ///		通常はこのAPIはPMlib内部で自動的に実行される
    ///
    void gather(void);


    /// Root区間をstop
    ///	@note
    ///		Stop the Root section, which means the end of PMlib stats recording
    ///
    void stopRoot (void);


    /// Count the number of shared measured sections
    ///
    ///   @param[out] nSections     number of shared measured sections
    ///
    void countSections (int &nSections);


    /// Check if the section was defined in serial or parallel region
    ///
    ///   @param[in]  id         shared section number
    ///   @param[out] mid        class private section number
    ///   @param[out] inside     0/1 (0:serial region / 1:parallel region)
    ///
    void SerialParallelRegion (int &id, int &mid, int &inside);


    ///  OpenMP並列処理されたPMlibスレッド測定区間のうち parallel regionから
    ///  呼び出された測定区間のスレッド測定情報をマスタースレッドに集約する。
    ///
    ///	 @param[in] id  section number
    ///
    ///   @note この測定区間の番号はスレッドプライベートな番号ではなく、
    ///		共通番号であることに注意。
    ///   @note  
    ///  通常このAPIはPMlib内部で自動的に実行され、利用者が呼び出す必要はない。
    ///
    void mergeThreads(int id);


    /// PMlibレポートの出力をコントロールする汎用ルーチン
    ///   @brief
    /// - [1] stop the Root section
    /// - [2] merge thread serial/parallel sections
    /// - [3] select the type of the report and start producing the report
    ///
    /// @param[in] FILE* fc     output file pointer
    ///
    ///   @note fcが"" (NULL)の場合は標準出力に出力される
    ///
    /// @note
    /// C++ プログラムで OpenMPパラレル構文の内側で測定区間を定義した場合は、
    /// PerfMonitorクラスのreport()ではなく、
    /// PerfReportクラスのreport()を呼び出す必要がある。
    ///
    void report(FILE* fp);



    /// 出力する性能統計レポートの種類を選択し、ファイルへの出力を開始する。
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///
    ///   @note
    ///   基本統計レポート、MPIランク別詳細レポート、HWPC統計情報の詳細レポート、
    ///   スレッド別詳細レポートをまとめて出力する。
	///   出力する内容は環境変数 PMLIB_REPORTの指定によってコントロールする。
	///   PMLIB_REPORT=BASIC : 基本統計レポートを出力する。
	///   PMLIB_REPORT=DETAIL: MPIランク別に経過時間、頻度、HWPC統計情報の詳細レポートを出力する。
	///   PMLIB_REPORT=FULL： BASICとDETAILのレポートに加えて、
	///		各MPIランクが生成した各並列スレッド毎にHWPC統計情報の詳細レポートを出力する。
    ///   @note  
    ///  通常このAPIはPMlib内部で自動的に実行され、利用者が呼び出す必要はない。
    ///
    void selectReport(FILE* fp);


    /// 測定結果の基本統計レポートを出力。
    ///   排他測定区間毎に出力。プロセスの平均値、ジョブ全体の統計値も出力。
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
    ///   @param[in] comments 任意のコメント
    ///   @param[in] op_sort    測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    ///   @note 基本統計レポートは排他測定区間, 非排他測定区間をともに出力する。
    ///      MPIの場合、rank0プロセスの測定回数が１以上の区間のみを表示する。
    ///   @note  内部では以下の処理を行う。
    ///      各測定区間の全プロセス途中経過状況を集約。 gather_and_stats();
    ///      測定結果の平均値・標準偏差などの基礎的な統計計算。
    ///      経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
    ///      各測定区間のHWPCイベントの統計値を取得する。
    ///
    void print(FILE* fp, const std::string hostname, const std::string comments, int op_sort=0);


    /// MPIランク別詳細レポート、HWPC詳細レポートを出力。
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] legend   HWPC 記号説明の表示 (0:なし、1:表示する)
    ///   @param[in] op_sort    測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    ///   @note 詳細レポートは排他測定区間のみを出力する
    ///
    void printDetail(FILE* fp, int legend=0, int op_sort=0);


    /// 指定プロセスに対してスレッド別詳細レポートを出力。
    ///
    ///   @param[in] fp	       出力ファイルポインタ
    ///   @param[in] rank_ID   出力対象プロセスのランク番号
    ///   @param[in] op_sort     測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    void printThreads(FILE* fp, int rank_ID=0, int op_sort=0);


    /// HWPC 記号の説明表示を出力。
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///
    void printLegend(FILE* fp);


    /// プロセスグループ単位でのMPIランク別詳細レポート、HWPC詳細レポート出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] p_group  MPI_Group型 groupのgroup handle
    ///   @param[in] p_comm   MPI_Comm型 groupに対応するcommunicator
    ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
    ///   @param[in] group  int型 (省略可)プロセスグループ番号
    ///   @param[in] legend int型 (省略可)HWPC記号説明の表示(0:なし、1:表示する)
    ///   @param[in] op_sort int型 (省略可)測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    ///   @note プロセスグループはp_group によって定義され、p_groupの値は
    ///   MPIライブラリが内部で定める大きな整数値を基準に決定されるため、
    ///   利用者にとって識別しずらい場合がある。
    ///   別に1,2,3,..等の昇順でプロセスグループ番号 groupをつけておくと
    ///   レポートが識別しやすくなる。
    ///
    void printGroup(FILE* fp, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group=0, int legend=0, int op_sort=0);


    /// MPI communicatorから自動グループ化したMPIランク別詳細レポート、HWPC詳細レポートを出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] p_comm   MPI_Comm型 MPI_Comm_split()で対応つけられたcommunicator
    ///   @param[in] icolor int型 MPI_Comm_split()のカラー変数
    ///   @param[in] key    int型 MPI_Comm_split()のkey変数
    ///   @param[in] legend int型 (省略可)HWPC記号説明の表示(0:なし、1:表示する)
    ///   @param[in] op_sort int型 (省略可)測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    void printComm (FILE* fp, MPI_Comm p_comm, int icolor, int key, int legend=0, int op_sort=0);


    /// 測定途中経過の状況レポートを出力（排他測定区間を対象とする）
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] comments 任意のコメント
    ///   @param[in] op_sort 測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    ///   @note 基本レポートと同様なフォーマットで途中経過を出力する。
    ///      多数回の反復計算を行う様なプログラムにおいて初期の経過状況を
    ///      モニターする場合などに有効に用いることができる。
    ///
    void printProgress(FILE* fp, const std::string comments, int op_sort=0);



    /// ポスト処理用traceファイルの出力
    ///
    /// @note プログラム実行中一回のみポスト処理用traceファイルを出力できる
    /// 現在サポートしているフォーマットは OTF(Open Trace Format) v1.1
    ///
    void postTrace(void);


    /// 旧バージョンとの互換維持用(並列モードを設定)。
    /// 利用者は通常このAPIを呼び出す必要はない。
    ///
    /// @param[in] p_mode 並列モード
    /// @param[in] n_thread
    /// @param[in] n_proc
    ///
    /// @note 並列モードはPMlib内部で自動的に識別可能なため、
    ///       利用者は通常このAPIを呼び出す必要はない。
    ///
    void setParallelMode(const std::string& p_mode, const int n_thread, const int n_proc);


    /// 旧バージョンとの互換維持用(ランク番号の通知)
    /// 利用者は通常このAPIを呼び出す必要はない。
    ///
    /// @param[in] my_rank_ID   MPI process ID
    ///
    /// @note ランク番号はPMlib内部で自動的に識別される。
    /// @note 並列モードはPMlib内部で自動的に識別可能なため、
    ///       利用者は通常このAPIを呼び出す必要はない。
    ///
    void setRankInfo(const int my_rank_ID) {
      //	my_rank = my_rank_ID;
    }

    /// 旧バージョンとの互換維持用(PMlibバージョン番号の文字列を返す)
    /// 利用者は通常このAPIを呼び出す必要はない。
    ///
    std::string getVersionInfo(void);


  private:

    /// 測定区間のラベルに対応する区間番号を追加作成する
    /// Add a new entry in the section map (section name, section ID)
    ///
    ///   @param[in] arg_st   the label of the newly created section
    ///
    ///	  @return the section ID
    ///
    int add_section_object(std::string arg_st);

    /// 測定区間のラベルに対応する区間番号を取得
    /// Search through the map and find section ID from the given label
    ///
    ///   @param[in] arg_st   the label of the section
    ///
    ///	  @return the section ID
    ///
    int find_section_object(std::string arg_st);

    /// 測定区間の区間番号に対応するラベルを取得
    /// Search the section ID in the map and return the label string
    ///
    ///   @param[in]  ip      the section ID
    ///   @param[out] p_label the label string of the section
    ///
    void loop_section_object(const int ip, std::string& p_label);

    /// 全測定区間のラベルと番号を登録順で表示
    /// Check print all the defined section IDs and labels
    ///
    void check_all_section_object(void);

    /// Add a new entry in the shared section map (section name, section ID)
    ///
    ///   @param[in] arg_st   the label of the newly created shared section
    ///
    ///	  @return the section ID
    ///
    int add_shared_section(std::string arg_st);

    /// Print all the shared section IDs and labels 
    ///
    void check_all_shared_sections(void);



    /// 全プロセスの測定中経過情報を集約
    ///
    ///   @note  以下の処理を行う。
    ///    各測定区間の全プロセス途中経過状況を集約。
    ///    測定結果の平均値・標準偏差などの基礎的な統計計算。
    ///    各測定区間のHWPCイベントの統計値を取得する。
    void gather_and_stats(void);

    /// 経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
    ///
    void sort_m_order(void);

    /// 基本統計レポートのヘッダ部分を出力。
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
    ///   @param[in] comments 任意のコメント
    ///   @param[in] tot      測定経過時間
    ///
    void printBasicHeader(FILE* fp, const std::string hostname, const std::string comments, double tot=0.0);

    /// 基本統計レポートの各測定区間を出力
    ///
    ///   @param[in] fp        出力ファイルポインタ
    ///   @param[in] maxLabelLen    ラベル文字長
    ///   @param[in] tot       全経過時間
    ///   @param[in] sum_time_flop  演算経過時間
    ///   @param[in] sum_time_comm  通信経過時間
    ///   @param[in] sum_time_other  その他経過時間
    ///   @param[in] sum_flop  演算量
    ///   @param[in] sum_comm  通信量
    ///   @param[in] sum_other その他
    ///   @param[in] unit      計算量の単位
    ///   @param[in] op_sort     測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    ///   @note   計算量（演算数やデータ移動量）選択方法は PerfWatch::stop() の
    ///           コメントに詳しく説明されている。
    ///
    void printBasicSections(FILE* fp, int maxLabelLen, double& tot,
              double& sum_flop, double& sum_comm, double& sum_other,
              double& sum_time_flop, double& sum_time_comm, double& sum_time_other,
              std::string unit, int op_sort=0);

    /// 基本統計レポートのテイラー部分を出力。
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] maxLabelLen    ラベル文字長
    ///   @param[in] sum_time_flop  演算経過時間
    ///   @param[in] sum_time_comm  通信経過時間
    ///   @param[in] sum_time_other  その他経過時間
    ///   @param[in] sum_flop  演算量
    ///   @param[in] sum_comm  通信量
    ///   @param[in] sum_other  その他
    ///   @param[in] unit 計算量の単位
    ///
    void printBasicTailer(FILE* fp, int maxLabelLen,
              double sum_flop, double sum_comm, double sum_other,
              double sum_time_flop, double sum_time_comm, double sum_time_other,
              const std::string unit);


	/// Report the BASIC HWPC statistics for the master process
	///
	///   @param[in] fp       	report file pointer
	///   @param[in] maxLabelLen    maximum label string field length
	///   @param[in] op_sort 	sorting option (0:sorted by seconds, 1:listed order)
	///
	void printBasicHWPC (FILE* fp, int maxLabelLen, int op_sort=0);


	/// Report the BASIC power consumption statistics of the master node
	///
	///   @param[in] fp         report file pointer
	///   @param[in] maxLabelLen    maximum label field string length
    ///   @param[in] op_sort     sorting option (0:sorted by seconds, 1:listed order)
	///
	///		@note	remark that power consumption is reported per node, not per process
	///
	void printBasicPower(FILE* fp, int maxLabelLen, int op_sort=0);


    /// PerfMonitorクラス用エラーメッセージ出力
    ///
    ///   @param[in] func  関数名
    ///   @param[in] fmt  出力フォーマット文字列
    ///
    void printDiag(const char* func, const char* fmt, ...)
    {
      if (my_rank == 0) {
        fprintf(stderr, "\n\n*** PMlib message. PerfMonitor::%s: ", func );
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
      }
    }



	/// initialize POWER API context and obtain the objects
	/// default context and extended context
	///
	int initializePOWER (void);


	/// finalize POWER API contexts
	/// destroy the default context and extended context
	///
	int finalizePOWER (void);


	/// read or update the Power Knob values via POWER API
	///   @param[in] knob		power knob chooser
	///   @param[in] operation	type of operation [0:read, 1:update]
	///   @param[in/out] value	power knob value
	///
	int operatePowerKnob (int knob, int operation, int & value);



    /// just as a record of DEBUG version
    ///
    void Obsolete_mergeThreads(void);


  }; // end of class PerfMonitor //

  /**
   * additional class to integrate serial/threaded report for C++ user code
   */
  class PerfReport {
  public:

    /// PMlibレポートの出力をコントロールする汎用ルーチン。
    ///   @brief
    /// - [1] stop the Root section
    /// - [2] merge thread serial/parallel sections
    /// - [3] select the type of the report and start producing the report
    ///
    /// @param[in] FILE* fc     output file pointer. if fc is "" (NULL), then stdout is selected.
    ///
    /// @note
    /// C++ プログラムからPMlibを呼び出す場合についてだけ必要となることがある。
    /// @note
    /// C++ プログラムで、OpenMPパラレル構文の内側で測定区間を定義した場合は、
    /// PerfMonitorクラスのreport()ではなく、
    /// こちらのPerfReportクラスのreport()を呼び出す必要がある。
    ///
    void report(FILE* fp);

  }; // end of class PerfReport //

} /* namespace pm_lib */

#endif // _PM_PERFMONITOR_H_

