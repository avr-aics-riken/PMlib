/* ##################################################################
 *
 * PMlib - Performance Monitor library
 *
 * Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
 * All rights reserved.
 *
 * Copyright (c) 2012-2015 Advanced Institute for Computational Science, RIKEN.
 * All rights reserved.
 *
 * ###################################################################
 */

//! @file   PerfWatch.cpp
//! @brief  PerfWatch class

// When compiling with USE_PAPI macro, openmp option should be enabled.

#ifdef _PM_WITHOUT_MPI_
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif

#include "PerfWatch.h"
#include <cmath>

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

struct pmlib_papi_chooser papi;
struct hwpc_group_chooser hwpc_group;

extern void sortPapiCounterList ();
extern void outputPapiCounterHeader (FILE*, std::string);
extern void outputPapiCounterList (FILE*);
extern void outputPapiCounterLegend (FILE*);
extern double countPapiByte (pmlib_papi_chooser );


namespace pm_lib {

  
  bool PerfWatch::ExclusiveStarted = false;
  
  
  /// 単位変換.
  ///
  ///   @param[in] fops 浮動小数演算数/通信量(バイト)
  ///   @param[out] unit 単位の文字列
  ///   @param[in] typeCalc 測定対象タイプ (0:通信, 1:計算)
  ///   @return  単位変換後の数値
  ///
  double PerfWatch::unitFlop(double fops, std::string &unit, const int typeCalc)
  {
    int is_unit;
    if (typeCalc == 0) is_unit=0;
    if (typeCalc == 1) is_unit=1;
    if (hwpc_group.number[I_bandwidth] > 0) is_unit=0;
    if (hwpc_group.number[I_flops] > 0) is_unit=1;

    double P, T, G, M, K, ret=0.0;
    K = 1000.0;
    M = 1000.0*K;
    G = 1000.0*M;
    T = 1000.0*G;
    P = 1000.0*T;

    if (is_unit == 0) {
      if      ( fops > P ) {
        ret = fops / P;
        unit = "PB/sec";
      }
      else if ( fops > T ) {
        ret = fops / T;
        unit = "TB/sec";
      }
      else if ( fops > G ) {
        ret = fops / G;
        unit = "GB/sec";
      }
      else {
        ret = fops / M;
        unit = "MB/sec";
      }
    }

    if (is_unit == 1) {
      if      ( fops > P ) {
        ret = fops / P;
        unit = "Pflops";
      }
      else if ( fops > T ) {
        ret = fops / T;
        unit = "Tflops";
      }
      else if ( fops > G ) {
        ret = fops / G;
        unit = "Gflops";
      }
      else {
        ret = fops / M;
        unit = "Mflops";
      }
    }

    return ret;
  }

  

  /// HWPCによるイベントカウンターの測定値を PAPI APIで取得収集する
  ///
  ///
  void PerfWatch::gatherHWPC()
  {
#ifdef USE_PAPI
	if (!m_exclusive) return;
	if ( papi.num_events == 0 ) return;

	sortPapiCounterList ();

    int is_unit = statsSwitch();

	// if conditions meet, overwrite the user values with HWPC measured values
	if ( (is_unit == 2) || (is_unit == 3) )
	{
		m_flop = my_papi.v_sorted[my_papi.num_sorted-1] * m_time;
	}

#endif
  }
  
  
  /// 測定結果情報をノード０に集約.
  ///
  void PerfWatch::gather()
  {
    if (m_gathered) {
      printError("PerfWatch::gather()",  "already gathered\n");
      PM_Exit(0);
    }
    
    int m_np;
    m_np = num_process;
    
    if (!(m_timeArray  = new double[m_np]))        PM_Exit(0);
    if (!(m_flopArray  = new double[m_np]))        PM_Exit(0);
    if (!(m_countArray = new unsigned long[m_np])) PM_Exit(0);
    
    if ( m_np == 1 ) {
      m_timeArray[0] = m_time;
      m_flopArray[0] = m_flop;
      m_countArray[0]= m_count;
      m_count_sum = m_count;
    } else {
      if ( MPI_Gather(&m_time,  1, MPI_DOUBLE,        m_timeArray,  1, MPI_DOUBLE,        0, MPI_COMM_WORLD) != MPI_SUCCESS ) PM_Exit(0);
      if ( MPI_Gather(&m_flop,  1, MPI_DOUBLE,        m_flopArray,  1, MPI_DOUBLE,        0, MPI_COMM_WORLD) != MPI_SUCCESS ) PM_Exit(0);
      if ( MPI_Gather(&m_count, 1, MPI_UNSIGNED_LONG, m_countArray, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD) != MPI_SUCCESS ) PM_Exit(0);
      if ( MPI_Allreduce(&m_count, &m_count_sum, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD) != MPI_SUCCESS ) PM_Exit(0);
    }
    m_gathered = true;
  }

  
  /// Statistics among processes
  /// Translate in Japanese later on...
  /// 測定結果の平均値・標準偏差などの基礎的な統計計算
  ///
  void PerfWatch::statsAverage()
  {

    int m_np;
    m_np = num_process;

    if (my_rank == 0) {
      if (m_exclusive) {
        
        // check call count
        int n = m_countArray[0];
        for (int i = 1; i < m_np; i++) {
          if (m_countArray[i] != n) m_valid = false;
        }
        // 排他測定(m_exclusive==true)では全ノードで測定回数が等しいことを仮定.
        // 等しくない場合(m_valid==fals)には、統計計算をスキップする.
        //	if (!m_valid) return;
        //
        // version 4 以降の注意
        // 計算が複雑になり負荷のアンバランスが生じると、区間の呼び出し回数は
        // プロセス毎に異なる場合がありえる。
        // このため変数 m_valid の意味合いは当初の設計からは修正が必要となる。
        //
        
        // 平均値
        m_time_av = 0.0;
        m_flop_av = 0.0;
        for (int i = 0; i < m_np; i++) {
          m_time_av += m_timeArray[i];
          m_flop_av += m_flopArray[i];
        }
        m_time_av /= m_np;
        m_flop_av /= m_np;
        
        // 標準偏差
        m_time_sd = 0.0;
        m_flop_sd = 0.0;
        if (m_np > 1) {
          for (int i = 0; i < m_np; i++) {
            double d_time = m_timeArray[i] - m_time_av;
            double d_flop = m_flopArray[i] - m_flop_av;
            m_time_sd += d_time*d_time;
            m_flop_sd += d_flop*d_flop;
          }
          m_time_sd = sqrt(m_time_sd / (m_np-1));
          m_flop_sd = sqrt(m_flop_sd / (m_np-1));
        }
        
        // 通信の場合，各ノードの通信時間の最大値
        m_time_comm = 0.0;
        if (m_typeCalc == 0) {
          double comm_max = 0.0;
          for (int i = 0; i < m_np; i++) {
            if (m_timeArray[i] > comm_max) comm_max = m_timeArray[i];
          }
          m_time_comm = comm_max; 
        }
      } // m_exclusive
    } // my_rank == 0
  }
  
  
  /// 計算量の選択を行う
  ///
  /// @return  
  ///   0: ユーザが引数で指定した通信量を採用する "Bytes/sec"
  ///   1: ユーザが引数で指定した計算量を採用する "Flops"
  ///   2: HWPC が自動的に測定する通信量を採用する	"Bytes/s (HWPC)"
  ///   3: HWPC が自動的に測定する計算量を用いる	"Flops (HWPC)"
  ///   4: HWPC が自動的に測定する他の測定量を用いる "events (HWPC)"
  ///
  /// @note
  /// 計算量としてユーザー申告値を用いるかHWPC計測値を用いるかの決定を行う
  /// もし環境変数HWPC_CHOOSERの値がFLOPSかBANDWIDTHの場合はそれが優先される
  /// 必要であればHWPC計測値を m_flopArray[i]にコピー(上書き)する
  ///
  int PerfWatch::statsSwitch()
  {
    int is_unit;
    // 0: user set bandwidth
    // 1: user set flop counts
    // 2: HWPC base bandwidth
    // 3: HWPC base flop counts
    // 4: other HWPC base statistics

    is_unit=-1;

    if (m_typeCalc == 0) { is_unit=0; }
    if (m_typeCalc == 1) { is_unit=1; }
#ifdef USE_PAPI
    if (hwpc_group.number[I_bandwidth] > 0) { is_unit=2; }
    if (hwpc_group.number[I_flops]  > 0) { is_unit=3; }
    if (( hwpc_group.number[I_vector]
        + hwpc_group.number[I_cache]
        + hwpc_group.number[I_cycle]) >0) { is_unit=4; }
#endif

    return is_unit;
  }

  
  /// 時刻を取得.
  ///
  ///   Unix/Linux: gettimeofdayシステムコールを使用.
  ///   Windows: GetSystemTimeAsFileTime API(sph_win32_util.h)を使用.
  ///
  ///   @return 時刻値(秒)
  ///
  double PerfWatch::getTime()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
  }
 
  
  /// MPIランク別測定結果を出力.
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] totalTime 全排他測定区間での計算時間(平均値)の合計
  ///
  ///   @note ノード0からのみ呼び出し可能。ノード毎に非排他測定区間も出力

  void PerfWatch::printDetailRanks(FILE* fp, double totalTime)
  {
    
    int m_np;
    m_np = num_process;
    
    double t_per_call, perf_rate;
    double tMax = 0.0;
    for (int i = 0; i < m_np; i++) {
      tMax = (m_timeArray[i] > tMax) ? m_timeArray[i] : tMax;
    }
    
    std::string unit;
    int is_unit = statsSwitch();
    if (is_unit == 0) unit = "Bytes/sec";	// 0: user set bandwidth
    if (is_unit == 1) unit = "Flops";		// 1: user set flop counts
    if (is_unit == 2) unit = "Bytes/s (HWPC)";	// 2: HWPC base bandwidth
    if (is_unit == 3) unit = "Flops (HWPC)";	// 3: HWPC base flop counts
    if (is_unit == 4) unit = "events (HWPC)";	// 4: other HWPC base statistics


    unsigned long total_count = 0;
    for (int i = 0; i < m_np; i++) total_count += m_countArray[i];
    
    if ( total_count > 0 && is_unit <= 3) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
      fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   flop|msg    speed              \n");
      for (int i = 0; i < m_np; i++) {
		t_per_call = (m_countArray[i]==0) ? 0.0: m_timeArray[i]/m_countArray[i];
		perf_rate = (m_countArray[i]==0) ? 0.0 : m_flopArray[i]/m_timeArray[i];
		fprintf(fp, "Rank %5d : %8ld  %9.3e  %5.1f  %9.3e  %9.3e  %9.3e  %9.3e %s\n",
			i, 
			m_countArray[i], // コール回数
			m_timeArray[i],  // ノードあたりの時間
			100*m_timeArray[i]/totalTime, // 非排他測定区間に対する割合
			tMax-m_timeArray[i], // ノード間の最大値を基準にした待ち時間
			t_per_call,      // 1回あたりの時間コスト
			m_flopArray[i],  // ノードあたりの演算数
			perf_rate,       // スピード　Bytes/sec or Flops
			unit.c_str()     // スピードの単位
			);
      }
    } else if ( total_count > 0 && is_unit > 3) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
      fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   \n");
      for (int i = 0; i < m_np; i++) {
		t_per_call = (m_countArray[i]==0) ? 0.0: m_timeArray[i]/m_countArray[i];
		fprintf(fp, "Rank %5d : %8ld  %9.3e  %5.1f  %9.3e  %9.3e  \n",
			i, 
			m_countArray[i], // コール回数
			m_timeArray[i],  // ノードあたりの時間
			100*m_timeArray[i]/totalTime, // 非排他測定区間に対する割合
			tMax-m_timeArray[i], // ノード間の最大値を基準にした待ち時間
			t_per_call      // 1回あたりの時間コスト
			);
      }
    }
  }


  ///   Groupに含まれるMPIランク別測定結果を出力.
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] totalTime 全排他測定区間での計算時間(平均値)の合計
  ///   @param[in] p_group プロセスグループ番号。0の時は全プロセスを対象とする。
  ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
  ///
  ///   @note ノード0からのみ呼び出し可能
  ///
  void PerfWatch::printGroupRanks(FILE* fp, double totalTime, MPI_Group p_group, int* pp_ranks)
  {
    int ip;
    int m_np;
    int new_id;
    if (p_group == 0) { // p_group should have some positive value
      fprintf(stderr, "*** error PerfWatch::printGroupRanks p_group is 0\n");
    }

    MPI_Group_size(p_group, &m_np);
    MPI_Group_rank(p_group, &new_id);
#ifdef DEBUG_PRINT_PAPI
    if (my_rank == 0) {
      fprintf(fp, "<printGroupRanks> pp_ranks[] has %d ranks\n", m_np);
      for (int i = 0; i < m_np; i++) {
		fprintf(fp, "%3d ", pp_ranks[i]);
      }
    }
#endif

    
    double t_per_call, perf_rate;
    double tMax = 0.0;
    for (int i = 0; i < m_np; i++) {
      tMax = (m_timeArray[pp_ranks[i]] > tMax) ? m_timeArray[pp_ranks[i]] : tMax;
    }
    
    std::string unit;
    int is_unit = statsSwitch();
    if (is_unit == 0) unit = "Bytes/sec";	// 0: user set bandwidth
    if (is_unit == 1) unit = "Flops";		// 1: user set flop counts
    if (is_unit == 2) unit = "Bytes/s (HWPC)";	// 2: HWPC base bandwidth
    if (is_unit == 3) unit = "Flops (HWPC)";	// 3: HWPC base flop counts
    if (is_unit == 4) unit = "events (HWPC)";	// 4: other HWPC base statistics


    unsigned long total_count = 0;
    for (int i = 0; i < m_np; i++) total_count += m_countArray[pp_ranks[i]];
    
    if ( total_count > 0 && is_unit <= 3) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
      fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   flop|msg    speed              \n");
      for (int i = 0; i < m_np; i++) {
	ip = pp_ranks[i];
	t_per_call = (m_countArray[ip]==0) ? 0.0: m_timeArray[ip]/m_countArray[ip];
	perf_rate = (m_countArray[ip]==0) ? 0.0 : m_flopArray[ip]/m_timeArray[ip];
	fprintf(fp, "Rank %5d : %8ld  %9.3e  %5.1f  %9.3e  %9.3e  %9.3e  %9.3e %s\n",
			ip, 
			m_countArray[ip], // コール回数
			m_timeArray[ip],  // ノードあたりの時間
			100*m_timeArray[ip]/totalTime, // 非排他測定区間に対する割合
			tMax-m_timeArray[ip], // ノード間の最大値を基準にした待ち時間
			t_per_call,      // 1回あたりの時間コスト
			m_flopArray[ip],  // ノードあたりの演算数
			perf_rate,       // スピード　Bytes/sec or Flops
			unit.c_str()     // スピードの単位
			);
      }
    } else if ( total_count > 0 && is_unit > 3) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
      fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   \n");
      for (int i = 0; i < m_np; i++) {
	ip = pp_ranks[i];
	t_per_call = (m_countArray[ip]==0) ? 0.0: m_timeArray[ip]/m_countArray[ip];
	fprintf(fp, "Rank %5d : %8ld  %9.3e  %5.1f  %9.3e  %9.3e  \n",
			ip, 
			m_countArray[ip], // コール回数
			m_timeArray[ip],  // ノードあたりの時間
			100*m_timeArray[ip]/totalTime, // 非排他測定区間に対する割合
			tMax-m_timeArray[ip], // ノード間の最大値を基準にした待ち時間
			t_per_call      // 1回あたりの時間コスト
			);
      }
    }
  }



  /// PAPI HWPC測定結果を区間毎に出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] s_label 区間のラベル
  ///
  ///   @note ノード0からのみ呼び出し可能
  ///
  void PerfWatch::printDetailHWPCsums(FILE* fp, std::string s_label)
  {
#ifdef USE_PAPI
    char* c_env = std::getenv("HWPC_CHOOSER");
    if (c_env == NULL) return;
    if (papi.num_events == 0) return;
    if (!m_exclusive) return;
    #ifdef DEBUG_PRINT_PAPI
    fprintf(fp, "\t<printDetailHWPCsums> my_rank=%d, m_count=%lu, m_count_sum=%lu\n", my_rank, m_count, m_count_sum);
    #endif
    if ( m_count_sum == 0 ) return;
    if (my_rank == 0) outputPapiCounterHeader (fp, s_label);
    outputPapiCounterList (fp);
#endif
  }


 
  ///   Groupに含まれるMPIプロセスのHWPC測定結果を区間毎に出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] s_label 区間のラベル
  ///   @param[in] p_group プロセスグループ番号。0の時は全プロセスを対象とする。
  ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
  ///
  ///   @note ノード0からのみ呼び出し可能
  ///
  void PerfWatch::printGroupHWPCsums(FILE* fp, std::string s_label, MPI_Group p_group, int* pp_ranks)
  {
#ifdef USE_PAPI
    char* c_env = std::getenv("HWPC_CHOOSER");
    if (c_env == NULL) return;
    if (papi.num_events == 0) return;
    if (!m_exclusive) return;
    if ( m_count_sum == 0 ) return;
    if (my_rank == 0) outputPapiCounterHeader (fp, s_label);
    outputPapiCounterGroup (fp, p_group, pp_ranks);

#endif
  }



  /// PAPI HWPCヘッダーの表示
  ///   @param[in] fp 出力ファイルポインタ
  ///
  void PerfWatch::printHWPCHeader(FILE* fp)
  {
#ifdef USE_PAPI
	const PAPI_hw_info_t *hwinfo = NULL;
	using namespace std;
	char* c_env = std::getenv("HWPC_CHOOSER");

	if (c_env == NULL) {
	  fprintf(fp, "\tHWPC_CHOOSER environment variable was not given.");
	  fprintf(fp, " So there will be no HWPC output.\n");
      return;
    } else {
	  fprintf(fp, "\tHWPC_CHOOSER=%s statistics were collected.\n", c_env);
	  fprintf(fp, "\tThe values of each process are the sum of threads.\n\n");
    }

#endif
  }


  /// PAPI HWPC Legendの表示
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///
  void PerfWatch::printHWPCLegend(FILE* fp)
  {
#ifdef USE_PAPI
	char* c_env = std::getenv("HWPC_CHOOSER");
	if (c_env == NULL) return;

	outputPapiCounterLegend (fp);
#endif
  }
  


  /// エラーメッセージ出力.
  ///
  ///   @param[in] func メソッド名
  ///   @param[in] fmt  出力フォーマット文字列
  ///
  void PerfWatch::printError(const char* func, const char* fmt, ...)
  {
    if (my_rank == 0) {
      fprintf(stderr, "%s error: \"%s\" ", func, m_label.c_str());
      va_list ap;
      va_start(ap, fmt);
      vfprintf(stderr, fmt, ap);
      va_end(ap);
    }
  }
  
  

  /// 測定区間スタート.
  ///
  void PerfWatch::start()
  {
	//	fprintf (stderr, "\t<PerfWatch::start> %s\n", m_label.c_str());
    if (m_label.empty()) {
      printError("PerfWatch::start()",  "properties not set\n");
      PM_Exit(0);
    }
    if (m_started) {
      printError("PerfWatch::start()",  "already started\n");
      PM_Exit(0);
    }
    if (m_exclusive && ExclusiveStarted) {
      printError("PerfWatch::start()",  "exclusive sections overlapped\n");
      PM_Exit(0);
    }
    m_started = true;
    if (m_exclusive) ExclusiveStarted = true;
    m_startTime = getTime();

#ifdef USE_PAPI
  #ifdef DEBUG_PRINT_PAPI
    if ((my_rank == 0) && (m_is_first))
		fprintf (stderr, "\t<PerfWatch::start> The first <%s> num_events=%d\n",
						m_label.c_str(), my_papi.num_events);
  #endif

	if (m_is_first) {
		my_papi = papi;
		m_is_first = false;
	}
	for (int i=0; i<papi.num_events; i++){
		my_papi.values[i] = 0;
	}

	#pragma omp parallel
	{
		struct pmlib_papi_chooser th_papi = my_papi;

		int i_ret = my_papi_bind_start (th_papi.events, th_papi.values, th_papi.num_events);
		if ( i_ret != PAPI_OK ) {
			int i_thread = omp_get_thread_num();
			fprintf(stderr, "*** error. <my_papi_bind_start> code: %d, thread:%d\n", i_ret, i_thread);
			PM_Exit(0);
		}
	}

#endif // USE_PAPI
  }
  

  /// 測定区間ストップ.
  ///
  ///   @param[in] flopPerTask     測定区間の計算量(演算量Flopまたは通信量Byte)
  ///   @param[in] iterationCount  計算量の乗数（反復回数）
  ///
  ///   @note  計算量または通信量をユーザが引数で明示的に指定する場合は、
  ///          そのボリュームは１区間１回あたりでflopPerTask*iterationCount
  ///          として算出される。
  ///          引数とは関係なくHWPC内部で自動計測する場合もある。
  ///          レポート出力する情報の選択方法は以下の規則による。
  ///
    /*
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
        CALC             指定あり   時間、fPT引数によるFlops
        COMM             指定あり   時間、fPT引数によるByte/s
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
     */
  void PerfWatch::stop(double flopPerTask, unsigned iterationCount)
  {
    if (!m_started) {
      printError("PerfWatch::stop()",  "not started\n");
      PM_Exit(0);
    }

    m_time += getTime() - m_startTime;
    m_count++;
    m_started = false;
    if (m_exclusive) ExclusiveStarted = false;


#ifdef USE_PAPI
	#pragma omp parallel
	{
		struct pmlib_papi_chooser th_papi = my_papi;
		int i_ret = my_papi_bind_stop (th_papi.events, th_papi.values, th_papi.num_events);
		if ( i_ret != PAPI_OK ) {
			int i_thread = omp_get_thread_num();
			fprintf(stderr, "*** error. <PerfWatch::stop> <my_papi_bind_stop> code: %d, thread:%d\n", i_ret, i_thread);
			PM_Exit(0);
		}
		#pragma omp critical
		{
		for (int i=0; i<papi.num_events; i++) {
			my_papi.values[i] += th_papi.values[i];
			}
		}

		#ifdef DEBUG_PRINT_PAPI_THREADS
    	if (my_rank == 0) {
			#pragma omp critical
			{
			int i_thread = omp_get_thread_num();
			fprintf (stderr, "   values of thread: %d\n", i_thread);
			for (int i=0; i<papi.num_events; i++) {
				fprintf (stderr, "\t i:%d, th.value=%llu\n", i,th_papi.values[i]);}
			}
		}
		#endif
	}

	for (int i=0; i<papi.num_events; i++) {
		my_papi.accumu[i] += my_papi.values[i];
	}

	// ユーザが引数で妥当な計算量を明示的に指定したかどうかのチェック
	if (flopPerTask > 0.0 && iterationCount > 0) {
		// 計算量または通信量をユーザが引数で明示的に指定した
    	m_flop += (double)flopPerTask * iterationCount;
	} else {
		// 引数での指定がない
		// 自動計測されたHWPCイベントを後続の <sortPapiCounterList> で分析する
		;
	}

	#ifdef DEBUG_PRINT_PAPI
    if (my_rank == 0) {
		int n_thread = omp_get_max_threads();
		fprintf (stderr, "   my_papi : sum of %d threads \n", n_thread);
		for (int i=0; i<my_papi.num_events; i++) {
			fprintf (stderr, "  event i=%d, value=%llu, accumu=%llu\n", i, my_papi.values[i], my_papi.accumu[i]);
		}
		fprintf (stderr, "\t<PerfWatch::stop> <%s> flopPerTask=%f, iterationCount=%u, time=%f, flop=%.1f, count=%lu\n"
			, m_label.c_str(), flopPerTask, iterationCount, m_time, m_flop, m_count);
	}
	#endif

#else
    m_flop += (double)flopPerTask * iterationCount;
#endif

  }

} /* namespace pm_lib */

