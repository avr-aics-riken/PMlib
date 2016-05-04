#ifndef _PM_PERFMONITOR_H_
#define _PM_PERFMONITOR_H_

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

//! @file   PerfMonitor.h
//! @brief  PerfMonitor class Header
//! @version rev.5.0.0

#ifdef _PM_WITHOUT_MPI_
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
  
  /// 排他測定用マクロ
#define PM_TIMING__         if (PerfMonitor::TimingLevel > 0)
  
  
  /// 排他測定＋非排他測定用マクロ
#define PM_TIMING_DETAIL__  if (PerfMonitor::TimingLevel > 1)

  
  /**
   * 計算性能測定管理クラス.
   */
  class PerfMonitor {
  public:
    
    /// 測定対象タイプ
    enum Type {
      COMM,  ///< 通信（あるいはメモリ転送）
      CALC,  ///< 計算
    };
    
    /// 測定レベル制御変数.
    /// =0:測定なし/=1:排他測定のみ/=2:非排他測定も(ディフォルト)
    static unsigned TimingLevel;
    
  private:
    unsigned m_nWatch;         ///< 測定区間数
    bool m_gathered;           ///< 想定結果集計済みフラグ
    int num_threads;           ///< 並列スレッド数
    int num_process;           ///< 並列プロセス数
    int my_rank;               ///< 自ランク番号
    std::string parallel_mode; ///< 並列動作モード（"Serial", "OpenMP", "FlatMPI", "Hybrid"）
    PerfWatch* m_watchArray;   ///< 測定区間の配列
    PerfWatch  m_total;        ///< Root区間の別称。v5.0以降では使用しない
      // PerfWatchのインスタンスは全部で m_nWatch 生成され、その番号対応は以下
      // m_watchArray[0]  :PMlibが定義するRoot区間 
      // m_watchArray[1 .. m_nWatch] :ユーザーが定義する各区間

    unsigned *m_order;         ///< 経過時間でソートした測定区間のリストm_order[m_nWatch]
    int researved_nWatch;      ///< 測定区間用にリザーブされたブロックの大きさ
    bool is_MPI_enabled;	     ///< PMlibの対応動作可能フラグ:MPI
    bool is_OpenMP_enabled;	   ///< PMlibの対応動作可能フラグ:OpenMP
    bool is_PAPI_enabled;	     ///< PMlibの対応動作可能フラグ:PAPI
    bool is_OTF_enabled;	   ///< 対応動作可能フラグ:OTF tracing 出力

  public:
    /// コンストラクタ.
    PerfMonitor() : m_watchArray(0) {}
    
    /// デストラクタ.
    ~PerfMonitor() { if (m_watchArray) delete[] m_watchArray; }
    

    /// 初期化.
    /// 測定区間数分の測定時計を準備.
    /// 最初にinit_nWatch区間分を確保し、不足したら動的にinit_nWatch追加する
    /// 全計算時間用測定時計をスタート.
    /// @param[in] init_nWatch 最初に確保する測定区間数（C++では省略可能） 
    ///
    /// @note 測定区間数 m_nWatch は不足すると動的に増えていく
    ///
    void initialize (int init_nWatch=100);


    /// 並列モードを設定
    ///
    /// @param[in] p_mode 並列モード
    /// @param[in] n_thread
    /// @param[in] n_proc
    ///
    /// @note 並列モードはPMlib内部で自動的に識別可能なため、
    ///       利用者は通常このAPIを呼び出す必要はない。
    ///
    void setParallelMode(const std::string& p_mode, const int n_thread, const int n_proc);

    
    /// ランク番号の通知 : This function is obsolete and is silently ignored.
    ///
    /// @param[in] MPI process ID
    ///
    /// @note ランク番号はPMlib内部で自動的に識別される。
    /// @note This function is obsolete and is silently ignored.
    /// It stays here only for the compatibility with old versions.
    ///
    void setRankInfo(const int my_rank_ID) {
      //	my_rank = my_rank_ID;
      fprintf(stderr, "Remark. <setRankInfo> function is obsolete, and is silently ignored.\n");
    }

    
    /// 測定区間にプロパティを設定.
    ///
    ///   @param[in] label ラベルとなる文字列
    ///   @param[in] type  測定量のタイプ(COMM:通信, CALC:計算)
    ///   @param[in] exclusive 排他測定フラグ。bool型(省略時true)、
    ///                        Fortran仕様は整数型(0:false, 1:true)
    ///
    ///   @note labelラベル文字列は測定区間を識別するために用いる。
    ///   各ラベル毎に対応した区間番号を内部で自動生成する
    ///   最初に確保した区間数init_nWatchが不足したら動的にinit_nWatch追加する
    ///   第１引数は必須。第２引数は明示的な自己申告モードの場合に必須。
    ///   第３引数は省略可
    ///
    void setProperties(const std::string& label, Type type, bool exclusive=true);

    
    /// 測定区間スタート
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///
    ///
    void start (const std::string& label);

    
    /// 測定区間ストップ
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///   @param[in] flopPerTask 測定区間の計算量(演算量Flopまたは通信量Byte):省略値0
    ///   @param[in] iterationCount  計算量の乗数（反復回数）:省略値1
    ///
    ///   @note  引数とレポート出力情報の関連はPerfWatch.hのコメントに詳しく説明されている。
    void stop(const std::string& label, double flopPerTask=0.0, unsigned iterationCount=1);

    
    /// 全プロセスの全測定結果情報をマスタープロセス(0)に集約.
    ///
    ///   全計算時間用測定時計をストップ.
    ///
    void gather(void);

    
    /// 測定結果の基本統計レポートを出力。
    ///   排他測定区間毎に出力。プロセスの平均値、ジョブ全体の統計値も出力。
    ///
    ///   @param[in] fp       出力ファイルポインタ
    ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
    ///   @param[in] comments 任意のコメント
    ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
    ///
    ///   @note 基本統計レポートは排他測定区間, 非排他測定区間をともに出力する。
    ///   MPIの場合、rank0プロセスの測定回数が１以上の区間のみを表示する。
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


    /**
     * @brief PMlibバージョン番号の文字列を返す
     */
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
    	fprintf(stderr, "<add_perf_label> %s : %d\n", arg_st.c_str(), ip);
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
				//	fprintf(stderr, "\t\t <%s> : %d\n", p_label.c_str(), p_id);
				return;
			}
		}
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

  }; // end of class PerfMonitor //

} /* namespace pm_lib */

#endif // _PM_PERFMONITOR_H_

