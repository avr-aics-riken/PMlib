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
# Copyright (c) 2012-2018 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2018 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################
 */

//! @file   PerfMonitor.h
//! @brief  PerfMonitor class Header
//! @version rev.5.8

#ifdef DISABLE_MPI
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif

#include "PerfWatch.h"
#include <cstdio>
#include <cstdlib>
#include "pmVersion.h"
#include <map>
#include <list>


namespace pm_lib {

  /**
   * 計算性能測定管理クラス.
   */
  class PerfMonitor {
  public:

    /// 測定計算量のタイプ
    enum Type {
      COMM,  ///< 通信（あるいはメモリ転送）
      CALC,  ///< 演算
    };

  private:
    int m_nWatch;         ///< 測定区間数
    int num_threads;           ///< 並列スレッド数
    int num_process;           ///< 並列プロセス数
    int my_rank;               ///< 自ランク番号
    std::string parallel_mode; ///< 並列動作モード
      // {"Serial", "OpenMP", "FlatMPI", "Hybrid"}
    std::string env_str_hwpc;  ///< 環境変数HWPC_CHOOSERの値
      // {"FLOPS", "BANDWIDTH", "VECTOR", "CACHE", "user"}
    PerfWatch* m_watchArray;   ///< 測定区間の配列
      // PerfWatchのインスタンスは全部で m_nWatch 生成され、その番号対応は以下
      // m_watchArray[0]  :PMlibが定義するRoot区間
      // m_watchArray[1 .. m_nWatch] :ユーザーが定義する各区間

    unsigned *m_order;         ///< 経過時間でソートした測定区間のリストm_order[m_nWatch]
    int reserved_nWatch;      ///< 測定区間用にリザーブされたブロックの大きさ
    bool is_MPI_enabled;	     ///< PMlibの対応動作可能フラグ:MPI
    bool is_OpenMP_enabled;	   ///< PMlibの対応動作可能フラグ:OpenMP
    bool is_PAPI_enabled;	     ///< PMlibの対応動作可能フラグ:PAPI
    bool is_OTF_enabled;	   ///< 対応動作可能フラグ:OTF tracing 出力
    bool m_gathered;           ///< 測定結果集計済みフラグ
      // std::string last_started_label;	///< 最後に start された測定区間
      // 排他的区間がオーバーラップする間違った使い方をチェックするために使う
      // エラー処理用のデータ ->  処理が遅くなる & 手間がかかるので使わない

  public:
    /// コンストラクタ.
    PerfMonitor() : m_watchArray(0) {}

    /// デストラクタ.
    ~PerfMonitor() { if (m_watchArray) delete[] m_watchArray; }


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


    /// 測定区間にプロパティを設定.
    ///
    ///   @param[in] label ラベルとなる文字列
    ///   @param[in] type  測定計算量のタイプ(COMM:通信, CALC:演算)
    ///   @param[in] exclusive 排他測定フラグ。bool型(省略時true)、
    ///                        Fortran仕様は整数型(0:false, 1:true)
    ///
    ///   @note labelラベル文字列は測定区間を識別するために用いる。
    ///   各ラベル毎に対応した区間番号を内部で自動生成する
    ///   最初に確保した区間数init_nWatchが不足したら動的にinit_nWatch追加する
    ///   第１引数は必須。第２引数は明示的な自己申告モードの場合に必須。
    ///   第３引数は省略可
    ///
    void setProperties(const std::string& label, Type type=CALC, bool exclusive=true);


    /// 測定区間スタート
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///
    ///
    void start (const std::string& label);


    /// 測定区間ストップ
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///   @param[in] flopPerTask 測定区間の計算量(演算量Flopまたは通信量Byte)
    ///   					:省略値0
    ///   @param[in] iterationCount  計算量の乗数（反復回数）:省略値1
    ///
    ///   @note  計算量のボリュームは次のように算出される。 \n
    ///          (A) ユーザ申告モードの場合は １区間１回あたりで
    ///          flopPerTask*iterationCount \n
    ///          (B) HWPCによる自動算出モードの場合は引数とは関係なく
    ///          HWPC内部値を利用
    ///   @note  出力レポートに表示される計算量は測定のモード・引数の
    ///          組み合わせで以下の規則により決定される。
    ///   @verbatim
    /**
    (A) ユーザ申告モード
      - HWPC APIが利用できないシステムや環境変数HWPC_CHOOSERが指定
        されていないジョブでは自動的にユーザ申告モードで実行される。
      - ユーザ申告モードでは(1):setProperties() と(2):stop()への引数により
        出力内容が決定される。
      - (1) ::setProperties(区間名, type, exclusive)の第2引数typeが
        計算量のタイプを指定する。演算(CALC)タイプか通信(COMM)タイプか。
      - (2) ::stop (区間名, fPT, iC)の第2引数fPTは測定計算量。
        演算（浮動小数点演算）あるいは通信（MPI通信やメモリロードストア
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
    HWPC_CHOOSER    type引数        fP引数       基本・詳細レポート出力      HWPC詳細レポート出力
    ------------------------------------------------------------------------------------------
	NONE (無指定)   CALC            指定値       時間、fP引数によるFlops     なし
	NONE (無指定)   COMM            指定値       時間、fP引数によるByte/s    なし
    FLOPS           無視            無視         時間、HWPC自動計測Flops     FLOPSに関連するHWPC統計情報
    VECTOR          無視            無視         時間、HWPC自動計測SIMD率    VECTORに関連するHWPC統計情報
    BANDWIDTH       無視            無視         時間、HWPC自動計測Byte/s    BANDWIDTHに関連するHWPC統計情報
    CACHE           無視            無視         時間、HWPC自動計測L1$,L2$   CACHEに関連するHWPC統計情報
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


    ///  OpenMP並列処理されたPMlibスレッド測定区間のうち parallel regionから
    ///  呼び出された測定区間のスレッド測定情報をマスタースレッドに集約する。
    ///
    ///   @note  内部で全測定区間をcheckして該当する測定区間を選択する。
    ///
    void mergeThreads(void);


    /// 測定結果の基本統計レポートを出力。
    ///   排他測定区間毎に出力。プロセスの平均値、ジョブ全体の統計値も出力。
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
    ///   @param[in] comments 任意のコメント
    ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
    ///
    ///   @note 基本統計レポートは排他測定区間, 非排他測定区間をともに出力する。
    ///      MPIの場合、rank0プロセスの測定回数が１以上の区間のみを表示する。
    ///   @note  内部では以下の処理を行う。
    ///      各測定区間の全プロセス途中経過状況を集約。 gather_and_sort();
    ///      測定結果の平均値・標準偏差などの基礎的な統計計算。
    ///      経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
    ///      各測定区間のHWPCイベントの統計値を取得する。
    ///
    void print(FILE* fp, const std::string hostname, const std::string comments, int seqSections=0);


    /// MPIランク別詳細レポート、HWPC詳細レポートを出力。
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] legend   int型 (省略可) HWPC 記号説明の表示 (0:なし、1:表示する)
    ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
    ///
    ///   @note 本APIよりも先にPerfWatch::gather()を呼び出しておく必要が有る
    ///         HWPC値は各プロセス毎に子スレッドの値を合算して表示する
    ///   @note 詳細レポートは排他測定区間のみを出力する
    ///
    void printDetail(FILE* fp, int legend=0, int seqSections=0);


    /// 指定プロセスに対してスレッド別詳細レポートを出力。
    ///
    ///   @param[in] fp			出力ファイルポインタ
    ///   @param[in] rank_ID	出力対象プロセスのランク番号
    ///   @param[in] seqSections 測定区間の表示順 (0:経過時間順、1:登録順で表示)
    ///
    void printThreads(FILE* fp, int rank_ID=0, int seqSections=0);


    /// プロセスグループ単位でのMPIランク別詳細レポート、HWPC詳細レポート出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] p_group  MPI_Group型 groupのgroup handle
    ///   @param[in] p_comm   MPI_Comm型 groupに対応するcommunicator
    ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
    ///   @param[in] group  int型 (省略可)プロセスグループ番号
    ///   @param[in] legend int型 (省略可)HWPC記号説明の表示(0:なし、1:表示する)
    ///   @param[in] seqSections int型 (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
    ///
    ///   @note プロセスグループはp_group によって定義され、p_groupの値は
    ///   MPIライブラリが内部で定める大きな整数値を基準に決定されるため、
    ///   利用者にとって識別しずらい場合がある。
    ///   別に1,2,3,..等の昇順でプロセスグループ番号 groupをつけておくと
    ///   レポートが識別しやすくなる。
    ///
    void printGroup(FILE* fp, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group=0, int legend=0, int seqSections=0);


    /// MPI communicatorから自動グループ化したMPIランク別詳細レポート、HWPC詳細レポートを出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] p_comm   MPI_Comm型 MPI_Comm_split()で対応つけられたcommunicator
    ///   @param[in] icolor int型 MPI_Comm_split()のカラー変数
    ///   @param[in] key    int型 MPI_Comm_split()のkey変数
    ///   @param[in] legend int型 (省略可)HWPC記号説明の表示(0:なし、1:表示する)
    ///   @param[in] seqSections int型 (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
    ///
    void printComm (FILE* fp, MPI_Comm new_comm, int icolor, int key, int legend=0, int seqSections=0);


    /// 測定途中経過の状況レポートを出力（排他測定区間を対象とする）
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] comments 任意のコメント
    ///   @param[in] seqSections 測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    ///   @note 基本レポートと同様なフォーマットで途中経過を出力する。
    ///      多数回の反復計算を行う様なプログラムにおいて初期の経過状況を
    ///      モニターする場合などに有効に用いることができる。
    ///
    void printProgress(FILE* fp, const std::string comments, int seqSections=0);


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
    /// @param[in] MPI process ID
    ///
    /// @note ランク番号はPMlib内部で自動的に識別される。
    /// @note 並列モードはPMlib内部で自動的に識別可能なため、
    ///       利用者は通常このAPIを呼び出す必要はない。
    ///
    void setRankInfo(const int my_rank_ID) {
      //	my_rank = my_rank_ID;
    }

    /// 旧バージョンとの互換維持用(全プロセスの測定結果を集約)
    /// 利用者は通常このAPIを呼び出す必要はない。
    ///
    /// @note  以下の処理を行う。
    ///       各測定区間の全プロセスの測定結果情報をマスタープロセスにに集約。
    ///       測定結果の平均値・標準偏差などの基礎的な統計計算。
    ///       経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
    ///       各測定区間のHWPCイベントの統計値を取得する。
    ///
    /// @note 集約処理は必要な時にPMlib内部で自動的に実行されるため
    ///       利用者は通常このAPIを呼び出す必要はない。
    ///
    void gather(void);

    /// 旧バージョンとの互換維持用(PMlibバージョン番号の文字列を返す)
    /// 利用者は通常このAPIを呼び出す必要はない。
    ///
    std::string getVersionInfo(void);


  private:
    std::map<std::string, int > array_of_symbols;

    /// 測定区間のラベルに対応する区間番号を追加作成する
    ///
    ///   @param[in] 測定区間のラベル
    ///
    int add_perf_label(std::string arg_st)
    {
		int ip = m_nWatch;
    	// perhaps it is better to return ip showing the insert status.
		// sometime later...
    	array_of_symbols.insert( make_pair(arg_st, ip) );
		#ifdef DEBUG_PRINT_LABEL
    	fprintf(stderr, "<add_perf_label> [%s] [%d]\n", arg_st.c_str(), ip);
		#endif
    	return ip;
    }

    /// 測定区間のラベルに対応する区間番号を取得
    ///
    ///   @param[in] 測定区間のラベル
    ///
    int find_perf_label(std::string arg_st)
    {
    	int p_id;
    	if (array_of_symbols.find(arg_st) == array_of_symbols.end()) {
    		p_id = -1;
    	} else {
    		p_id = array_of_symbols[arg_st] ;
    	}
		#ifdef DEBUG_PRINT_LABEL
    	fprintf(stderr, "<find_perf_label> %s : %d\n", arg_st.c_str(), p_id);
		#endif
    	return p_id;
    }

    /// 測定区間の区間番号に対応するラベルを取得
    ///
    ///   @param[in] ip 測定区間の区間番号
    ///   @param[in] label ラベルとなる文字列
    ///
    void loop_perf_label(const int ip, std::string& p_label)
    {
		std::map<std::string, int>::const_iterator it;
		int p_id;

		for(it = array_of_symbols.begin(); it != array_of_symbols.end(); ++it) {
			p_label = it->first;
			p_id = it->second;
			if (p_id == ip) {
				return;
			}
		}
		// should not reach here
		fprintf(stderr, "<loop_perf_label> search failed. ip=%d\n", ip);
	}

    /// 全測定区間のラベルと番号を登録順で表示
    ///
    void check_all_perf_label(void)
    {
		std::map<std::string, int>::const_iterator it;
		std::string p_label;
		int p_id;
		fprintf(stderr, "<check_all_perf_label> internal map: label, value\n");
		//	fprintf(stderr, "\t\t %s, %d\n", "<Root Section>:", 0);
		for(it = array_of_symbols.begin(); it != array_of_symbols.end(); ++it) {
			p_label = it->first;
			p_id = it->second;
			fprintf(stderr, "\t\t <%s> : %d\n", p_label.c_str(), p_id);
		}
    }

    /// 全プロセスの測定中経過情報を集約
    ///
    ///   @note  以下の処理を行う。
    ///    各測定区間の全プロセス途中経過状況を集約。
    ///    測定結果の平均値・標準偏差などの基礎的な統計計算。
    ///    経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
    ///    各測定区間のHWPCイベントの統計値を取得する。
    void gather_and_sort(void);

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
    ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順)
    ///
    ///   @note   計算量（演算数やデータ移動量）選択方法は PerfWatch::stop() の
    ///           コメントに詳しく説明されている。
    ///
    void printBasicSections(FILE* fp, int maxLabelLen, double& tot,
              double& sum_flop, double& sum_comm, double& sum_other,
              double& sum_time_flop, double& sum_time_comm, double& sum_time_other,
              std::string unit, int seqSections=0);

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

    /// PerfMonitorクラス用エラーメッセージ出力
    ///
    ///   @param[in] func  関数名
    ///   @param[in] fmt  出力フォーマット文字列
    ///
    void printDiag(const char* func, const char* fmt, ...)
    {
      if (my_rank == 0) {
        fprintf(stderr, "*** Error. PerfMonitor::%s: ", func );
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
      }
    }

  }; // end of class PerfMonitor //

} /* namespace pm_lib */

#endif // _PM_PERFMONITOR_H_

