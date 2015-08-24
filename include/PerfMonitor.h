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
//! @version rev.4.1.2

#ifdef _PM_WITHOUT_MPI_
#include "mpi_stubs.h"
#endif

#include "PerfWatch.h"
#include <cstdio>
#include <cstdlib>
#include "pmVersion.h"
#include <map>


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
      COMM,  ///< 0:通信
      CALC,  ///< 1:計算
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
    PerfWatch* m_watchArray;   ///< 測定時計配列
    PerfWatch  m_total;        ///< 全計算時間用測定時計
    // 注意 PerfWatchのインスタンスは全部で m_nWatch + 1 生成される
    // ユーザーが定義する各区間 m_watchArray[m_nWatch] と
    // 全区間を含む合計用に pmlibが独自に作成する m_totalとがある
    unsigned *m_order;         ///< 経過時間でソートした測定区間のリストm_order[m_nWatch]
    int researved_nWatch;      ///< 測定区間用にリザーブされたブロックの大きさ
    bool is_MPI_enabled;	     ///< PMlibの対応動作可能フラグ:MPI
    bool is_OpenMP_enabled;	   ///< PMlibの対応動作可能フラグ:OpenMP
    bool is_PAPI_enabled;	     ///< PMlibの対応動作可能フラグ:PAPI

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
    void setParallelMode(const std::string& p_mode, const int n_thread, const int n_proc);

    
    /// ランク番号の通知
    ///
    /// @param[in] myID MPI process ID
    ///
    void setRankInfo(const int myID) {
      //	This function is redundant, and is now silently ignored.
      //	my_rank = myID;
    }

    
    /// 測定区間にプロパティを設定.
    ///
    ///   @param[in] label ラベルとなる文字列
    ///   @param[in] type  測定対象タイプ(0:COMM:通信, 1:CALC:計算)
    ///   @param[in] exclusive 排他測定フラグ。bool型(省略時true)、
    ///                        Fortran仕様は整数型(0:false, 1:true)
    ///
    ///   @note labelラベル文字列は測定区間を識別するために用いる。
    ///   各ラベル毎に対応したキー番号 key を内部で自動生成する
    ///   最初に確保した区間数init_nWatchが不足したら動的にinit_nWatch追加する
    ///
    void setProperties(const std::string& label, Type type, bool exclusive=true) {
      int key = add_perf_label(label);
      if (m_nWatch < 0) {
        fprintf(stderr, "\tPerfMonitor::setProperties() error. key=%u \n", key);
        PM_Exit(0);
      }
      #ifdef DEBUG_PRINT_LABEL
      fprintf(stderr, "<setProperties> %s type:%d key:%d\n",
                             label.c_str(), type, key);
      #endif

      m_nWatch++;
      if (m_nWatch > researved_nWatch) {
        PerfWatch* watch_more = new PerfWatch[2*researved_nWatch ];
        for (int i = 0; i < researved_nWatch; i++) {
            watch_more[i] = m_watchArray[i];
        }
        delete [] m_watchArray;
        m_watchArray = watch_more;
        watch_more = NULL;
        researved_nWatch = 2*researved_nWatch;
      }
      m_watchArray[key].setProperties(label, type, num_process, my_rank, exclusive);
    }

    
    /// 測定区間スタート
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///
    ///
    void start (const std::string& label) {
      int key = key_perf_label(label);
      m_watchArray[key].start();
		#ifdef DEBUG_PRINT_LABEL
    	fprintf(stderr, "<start> %s : %d\n", label.c_str(), key);
		#endif
    }
    
    /// 測定区間ストップ
    ///
    ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
    ///   @param[in] flopPerTask あたりの計算量(Flop)または通信量(Byte):省略値0
    ///   @param[in] iterationCount  「タスク」実行回数:省略値1
    ///
    void stop(const std::string& label, double flopPerTask=0.0, unsigned iterationCount=1) {
      int key = key_perf_label(label);
      m_watchArray[key].stop(flopPerTask, iterationCount);
		#ifdef DEBUG_PRINT_LABEL
    	fprintf(stderr, "<stop>  %s : %d\n", label.c_str(), key);
		#endif
    }
    
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
    ///
    ///   @note ノード0以外は, 呼び出されてもなにもしない
    ///
    void print(FILE* fp, const std::string hostname, const std::string comments);

    
    /// MPIランク別詳細レポート、HWPC詳細レポートを出力。 非排他測定区間も出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] legend HWPC 記号説明の表示(0:なし、1:表示する) (optional)
    ///
    ///   @note 本APIよりも先にPerfWatch::gather()を呼び出しておく必要が有る
    ///         HWPC値は各プロセス毎に子スレッドの値を合算して表示する
    ///
    void printDetail(FILE* fp, int legend=0);


    
    /// プロセスグループ単位でのMPIランク別詳細レポート、HWPC詳細レポート出力
    ///
    ///   @param[in] fp 出力ファイルポインタ
    ///   @param[in] p_group  MPI_Group型 groupのgroup handle
    ///   @param[in] p_comm   MPI_Comm型 groupに対応するcommunicator
    ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
    ///   @param[in] group    int型 プロセスグループ番号 (optional)
    ///   @param[in] legend   int型 HWPC 記号説明の表示(0:表示、1:無) (optional)
    ///
    ///   @note プロセスグループは呼び出しプログラムが定義する
    ///
    void printGroup(FILE* fp, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group=0, int legend=0);

    /**
     * @brief PMlibバージョン番号の文字列を返す
     */
    std::string getVersionInfo(void);


  private:
	std::map<std::string, int > array_of_symbols;

    /// labelに対応した計測区間のkey番号を追加作成する
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

    /// labelに対応するkey番号を取得
    ///
    ///   @param[in] 測定区間のラベル
    ///
    int key_perf_label(std::string arg_st)
    {
    	int pair_value;
    	if (array_of_symbols.find(arg_st) == array_of_symbols.end()) {
    		pair_value = -1;
    	} else {
    		pair_value = array_of_symbols[arg_st] ;
    	}
		#ifdef DEBUG_PRINT_LABEL
    	fprintf(stderr, "<key_perf_label> %s : %d\n", arg_st.c_str(), pair_value);
		#endif
    	return pair_value;
    }

  };

} /* namespace pm_lib */

#endif // _PM_PERFMONITOR_H_


