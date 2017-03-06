/*
###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2017 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2017 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################
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

#if defined (__sparcv9)						// K computer and FX100
	#include <fjcex.h>
#endif

extern void sortPapiCounterList ();
extern void outputPapiCounterHeader (FILE*, std::string);
extern void outputPapiCounterList (FILE*);
extern void outputPapiCounterLegend (FILE*);
extern double countPapiByte (pmlib_papi_chooser );


namespace pm_lib {

  struct pmlib_papi_chooser papi;
  struct hwpc_group_chooser hwpc_group;
  double cpu_clock_freq;        /// processor clock frequency, i.e. Hz
  double second_per_cycle;  /// real time to take each cycle

  bool PerfWatch::ExclusiveStarted = false;


  /// 単位変換.
  ///
  ///   @param[in] fops 浮動小数演算数/通信量(バイト)
  ///   @param[out] unit 単位の文字列
  ///   @param[in] typeCalc  測定対象タイプ(0:通信, 1:計算)
  ///   @param[in] is_unit ユーザー申告値かHWPC自動測定値かの指定
  ///              = 0: ユーザが引数で指定した通信量"Bytes/sec"
  ///              = 1: ユーザが引数で指定した計算量"Flops"
  ///              = 2: HWPC が自動測定する通信量"Bytes/sec (HWPC)"
  ///              = 3: HWPC が自動測定する計算量"Flops (HWPC)"
  ///              = 4: HWPC が自動測定する他の測定量"events (HWPC)"
  ///   @return  単位変換後の数値
  ///
  ///   @note is_unitは通常PerfWatch::statsSwitch()で事前に決定されている
  ///

  double PerfWatch::unitFlop(double fops, std::string &unit, const int typeCalc, int is_unit)
  {

    double P, T, G, M, K, ret=0.0;
    K = 1000.0;
    M = 1000.0*K;
    G = 1000.0*M;
    T = 1000.0*G;
    P = 1000.0*T;

    if ((is_unit == 0) || (is_unit == 2))  {
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

    if ((is_unit == 1) || (is_unit == 3))  {
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

    if ((is_unit == 4))  {
      if      ( fops > P ) {
        ret = fops / P;
        unit = "Peta/sec";
      }
      else if ( fops > T ) {
        ret = fops / T;
        unit = "Tera/sec";
      }
      else if ( fops > G ) {
        ret = fops / G;
        unit = "Giga/sec";
      }
      else {
        ret = fops / M;
        unit = "Mega/sec";
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

	if ( my_papi.num_events == 0) return;

    int is_unit = statsSwitch();
	if ( (is_unit == 0) || (is_unit == 1) ) {
		printError("gatherHWPC()",  "internal error. is_unit=%d \n", is_unit);
		PM_Exit(0);
	}

	sortPapiCounterList ();

    // Curent PMlib version does not collect HWPC in the inclusive sections.
	// See Development note in PerfWatch::stop()
	//	if (!m_exclusive) return;

	double w = my_papi.v_sorted[my_papi.num_sorted-1] ;	// rate
	m_flop = w * m_time;	// volume

	// The space is reserved only once as a fixed size array
	if ( m_sortedArrayHWPC == NULL) {
		m_sortedArrayHWPC = new double[num_process*my_papi.num_sorted];
		if (!(m_sortedArrayHWPC)) {
			printError("gatherHWPC()", "new memory failed. %d x %d x 8 \n",
			num_process, my_papi.num_sorted);
			PM_Exit(0);
		}
	}

	if ( num_process > 1 ) {
		int iret =
		MPI_Gather (my_papi.v_sorted, my_papi.num_sorted, MPI_DOUBLE,
					m_sortedArrayHWPC, my_papi.num_sorted, MPI_DOUBLE,
					0, MPI_COMM_WORLD);
		if ( iret != 0 ) {
			printError("gatherHWPC()", " MPI_Gather failed.\n");
			PM_Exit(0);
		}
	} else {
		//	m_sortedArrayHWPC = my_papi.v_sorted;

        for (int i = 0; i < my_papi.num_sorted; i++) {
			m_sortedArrayHWPC[i] = my_papi.v_sorted[i];
		}
	}
	#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) {
        fprintf(stderr, "\t<gatherHWPC> [%15s], w(rate)=%e, m_time=%e\n",
			m_label.c_str(), w, m_time );
    }
	#endif
#endif
  }


  /// 測定結果情報をノード０に集約.
  ///
  void PerfWatch::gather()
  {
	#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) {
        fprintf(stderr, "\t<gather> [%15s], m_time=%e, m_flop=%e, m_count=%lu\n",
			m_label.c_str(), m_time, m_flop, m_count );
    }
	#endif

    int m_np;
    m_np = num_process;

	// The space is reserved only once as a fixed size array
	if ( m_timeArray == NULL) m_timeArray  = new double[m_np];
	if ( m_flopArray == NULL) m_flopArray  = new double[m_np];
	if ( m_countArray == NULL) m_countArray  = new unsigned long[m_np];
	if (!(m_timeArray) || !(m_timeArray) || !(m_timeArray)) {
		printError("gatherHWPC()", "new memory failed. %d(process) x 3 x 8 \n",
		num_process);
		PM_Exit(0);
	}

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
  }


  /// Statistics among processes
  /// Translate in Japanese later on...
  /// 測定結果の平均値・標準偏差などの基礎的な統計計算
  /// 測定区間の呼び出し回数はプロセス毎に異なる場合がありえる
  ///
  void PerfWatch::statsAverage()
  {

    if (my_rank == 0) {
      //	if (m_exclusive) {


        // 平均値
        m_time_av = 0.0;
        m_flop_av = 0.0;
        for (int i = 0; i < num_process; i++) {
          m_time_av += m_timeArray[i];
          m_flop_av += m_flopArray[i];
        }
        m_time_av /= num_process;
        m_flop_av /= num_process;

        // 標準偏差
        m_time_sd = 0.0;
        m_flop_sd = 0.0;
        if (num_process > 1) {
          for (int i = 0; i < num_process; i++) {
            double d_time = m_timeArray[i] - m_time_av;
            double d_flop = m_flopArray[i] - m_flop_av;
            m_time_sd += d_time*d_time;
            m_flop_sd += d_flop*d_flop;
          }
          m_time_sd = sqrt(m_time_sd / (num_process-1));
          m_flop_sd = sqrt(m_flop_sd / (num_process-1));
        }

        // 通信の場合，各ノードの通信時間の最大値
        m_time_comm = 0.0;
        if (m_typeCalc == 0) {
          double comm_max = 0.0;
          for (int i = 0; i < num_process; i++) {
            if (m_timeArray[i] > comm_max) comm_max = m_timeArray[i];
          }
          m_time_comm = comm_max;
        }
      //	} // m_exclusive
    } // my_rank == 0
  }


  /// 計算量の選択を行う
  ///
  /// @return
  ///   0: ユーザが引数で指定した通信量を採用する "Bytes/sec"
  ///   1: ユーザが引数で指定した計算量を採用する "Flops"
  ///   2: HWPC が自動的に測定する通信量を採用する	"Bytes/sec (HWPC)"
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
    // 4: other HWPC base events

    is_unit=-1;

    if (m_typeCalc == 0) { is_unit=0; }
    if (m_typeCalc == 1) { is_unit=1; }
#ifdef USE_PAPI
// If HWPC is available, initializeHWPC() and createPapiCounterList()
// should have created the HWPC table

    if (hwpc_group.number[I_bandwidth] > 0) {
      is_unit=2;
    } else if (hwpc_group.number[I_flops]  > 0) {
      is_unit=3;
    } else if (( hwpc_group.number[I_vector]
               + hwpc_group.number[I_cache]
               + hwpc_group.number[I_cycle]) >0) {
      is_unit=4;
    } else {
      // No more switch for this version. return (-1)
    }
#endif

    return is_unit;
  }


  /// 時刻を取得
  ///
  /// If available, call precise timer function
  /// Otherwise, gettimeofdayシステムコールを使用.
  ///
  ///   @return 時刻値(秒)
  ///
  //   		Windows: GetSystemTimeAsFileTime API(sph_win32_util.h)を使用???
  ///
  double PerfWatch::getTime()
  {

#if defined (__sparcv9)						// K computer and FX100
	register double tval;
	tval = __gettod()*1.0e-6;
	return (tval);

#elif defined (__APPLE__)
  struct timeval tv;
  gettimeofday(&tv, 0);
  return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;

#elif defined(__x86_64__)					// Intel Xeon

    #if defined (__INTEL_COMPILER) || defined(__gnu_linux__)
    // Replace the following code segment using inline assembler
	unsigned long long tsc;
	unsigned int lo, hi;
	__asm __volatile__ ( "rdtsc" : "=a"(lo), "=d"(hi) );
	tsc = ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
	return ((double)tsc * second_per_cycle);

    #else			// other linux/unix
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
    #endif

#else			// Other platforms. Not tested. Let's assume gettimeofday().
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
#endif
  }

  void PerfWatch::read_cpu_clock_freq()
  // read_cpu_clock_freq() reads the cpu freqency from  /proc/cpuinfo, etc..
  {
#if defined (__APPLE__)
/*
  cpu_clock_freq=0.0;
  second_per_cycle=0.0;

  FILE *fp;
  char buf[256];

  if ((fp = popen("sysctl -n machdep.cpu.brand_string", "r")) == NULL) {
    fprintf(stderr, "Failure popen\n");
    exit(-1);
  }

  // 実行結果を受けとる
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    printf("%s", buf);
  }
  pclose(fp);

  // buf >> "Intel(R) Core(TM) i7-6567U CPU @ 3.30GHz"
  */

#elif defined(__x86_64__)					// Intel Xeon
    #if defined (__INTEL_COMPILER) || defined(__gnu_linux__)
    cpu_clock_freq=0.0;
    second_per_cycle=0.0;

    FILE *fp;
    double value;
    char buffer[1024];
    fp = fopen("/proc/cpuinfo","r");
    if (fp == NULL) {
    	printError("read_cpu_clock_freq()",  "Can not open /proc/cpuinfo \n");
    	return;
    }
    while (fgets(buffer, 1024, fp) != NULL) {
    	//	printf("%s", buffer);
    	// assumed format
    	if (!strncmp(buffer, "cpu MHz",7)) {
    		sscanf(buffer, "cpu MHz\t\t: %lf", &value);
    		// sscanf handles regexp such as: sscanf (buffer, "%[^\t:]", value);
    		cpu_clock_freq = (value * 1.0e6);
    		break;
    	}
    }
    fclose(fp);
    if (cpu_clock_freq == 0.0) {
    	printError("read_cpu_clock_freq()",  "Failed parsing /proc/cpuinfo \n");
    	return;
    }
    second_per_cycle = 1.0/(double)cpu_clock_freq;
    #endif
#endif
    return;
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
    if (is_unit == 0) unit = "B/sec";	// 0: user set bandwidth
    if (is_unit == 1) unit = "Flops";		// 1: user set flop counts
    if (is_unit == 2) unit = "B/sec (HWPC)";	// 2: HWPC base bandwidth
    if (is_unit == 3) unit = "Flops (HWPC)";	// 3: HWPC base flop counts
    if (is_unit == 4) unit = "events (HWPC)";	// 4: other HWPC base statistics


    unsigned long total_count = 0;
    for (int i = 0; i < m_np; i++) total_count += m_countArray[i];

    if ( total_count > 0 && is_unit <= 4) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
      // fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   flop|msg    speed              \n");
      fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   counter     speed              \n");
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
    } else if ( total_count > 0 && is_unit > 4) {
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
#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) {
      fprintf(fp, "<printGroupRanks> pp_ranks[] has %d ranks:", m_np);
      for (int i = 0; i < m_np; i++) {
		fprintf(fp, "%3d ", pp_ranks[i]);
      }
      fprintf(fp, "\n");
    }
#endif


    double t_per_call, perf_rate;
    double tMax = 0.0;
    for (int i = 0; i < m_np; i++) {
      tMax = (m_timeArray[pp_ranks[i]] > tMax) ? m_timeArray[pp_ranks[i]] : tMax;
    }

    std::string unit;
    int is_unit = statsSwitch();
    if (is_unit == 0) unit = "B/sec";	// 0: user set bandwidth
    if (is_unit == 1) unit = "Flops";		// 1: user set flop counts
    if (is_unit == 2) unit = "B/sec (HWPC)";	// 2: HWPC base bandwidth
    if (is_unit == 3) unit = "Flops (HWPC)";	// 3: HWPC base flop counts
    if (is_unit == 4) unit = "events (HWPC)";	// 4: other HWPC base statistics


    unsigned long total_count = 0;
    for (int i = 0; i < m_np; i++) total_count += m_countArray[pp_ranks[i]];

    if ( total_count > 0 && is_unit <= 4) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
      // fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   flop|msg    speed              \n");
      fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   counter     speed              \n");
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
    } else if ( total_count > 0 && is_unit > 4) {
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
    if (my_papi.num_events == 0) return;
    if (!m_exclusive) return;
    if ( m_count_sum == 0 ) return;
    if (my_rank == 0) outputPapiCounterHeader (fp, s_label);
    outputPapiCounterList (fp);
#endif
  }



  ///   Groupに含まれるMPIプロセスのHWPC測定結果を区間毎に出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] s_label 区間のラベル
  ///   @param[in] p_group プロセスグループ番号。
  ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
  ///
  ///   @note ノード0からのみ呼び出し可能
  ///
  void PerfWatch::printGroupHWPCsums(FILE* fp, std::string s_label, MPI_Group p_group, int* pp_ranks)
  {
#ifdef USE_PAPI
    char* c_env = std::getenv("HWPC_CHOOSER");
    if (c_env == NULL) return;
    if (my_papi.num_events == 0) return;
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
	  fprintf(fp, "\tThe environment variable HWPC_CHOOSER is not provided. No HWPC report.\n");
      return;
    } else {
	  fprintf(fp, "\tThe environment variable HWPC_CHOOSER=%s is provided.\n", c_env);
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
      fprintf(stderr, "*** Error. PerfWatch::%s [%s] ",
                      func, m_label.c_str());
      va_list ap;
      va_start(ap, fmt);
      vfprintf(stderr, fmt, ap);
      va_end(ap);
    }
    (void) MPI_Barrier(MPI_COMM_WORLD);
  }


  /// 測定区間にプロパティを設定.
  ///
  ///   @param[in] label     ラベル
  ///   @param[in] id       ラベルに対応する番号
  ///   @param[in] typeCalc  測定量のタイプ(0:通信, 1:計算)
  ///   @param[in] my_rank_ID      ランク番号
  ///   @param[in] exclusive 排他測定フラグ
  ///
  void PerfWatch::setProperties(const std::string& label, int id, int typeCalc, int nPEs, int my_rank_ID, bool exclusive)
  {
    m_label = label;
    m_id = id;
    m_typeCalc = typeCalc;
    m_exclusive =  exclusive;
    num_process = nPEs;
    my_rank = my_rank_ID;
#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) {
    fprintf(stderr, "<PerfWatch::setProperties> %s : id=%d, typeCalc=%d, nPEs=%d, my_rank_ID=%d, exclusive=%s\n", label.c_str(), id, typeCalc, nPEs, my_rank_ID, exclusive?"true":"false");
    }
#endif

    m_is_OTF = 0;
#ifdef USE_OTF
	// 環境変数OTF_TRACING が指定された場合
	// OTF_TRACING = none(default) | yes | on | full
    std::string s;
    char* c_env;
    c_env = std::getenv("OTF_TRACING");
    if (c_env != NULL) {
      s = c_env;
      if ((s == "off") || (s == "no") ) {
        m_is_OTF = 0;
      } else if ((s == "on") || (s == "yes") ) {
        m_is_OTF = 1;
      } else if ((s == "full")) {
        m_is_OTF = 2;
      }
      #ifdef DEBUG_PRINT_WATCH
      if (my_rank == 0) {
	    fprintf(stderr, "\t<getenv> OTF_TRACING=%s is provided.\n", c_env);
      }
      #endif
    }

#endif
  }



  /// ポスト処理用traceファイル出力用の初期化
  ///
  void PerfWatch::initializeOTF(void)
  {
#ifdef USE_OTF
    if (m_is_OTF == 0) return;

	// 環境変数 OTF_FILENAME が指定された場合
    std::string s;
    char* c_env = std::getenv("OTF_FILENAME");
    if (c_env != NULL) {
      s = c_env;
      otf_filename = s;
    } else {
      otf_filename = "pmlib_optional_otf_files";
    }
    double baseT = PerfWatch::getTime();
    my_otf_initialize(num_process, my_rank, otf_filename.c_str(), baseT);
#endif
  }


  /// 測定区間のラベル情報をOTF に出力
  ///
  ///   @param[in] label     ラベル
  ///   @param[in] id       ラベルに対応する番号
  ///
  void PerfWatch::labelOTF(const std::string& label, int id)
  {
#ifdef USE_OTF
    if (m_is_OTF == 0) return;

	int i_switch = statsSwitch();
    my_otf_event_label(num_process, my_rank, id+1, label.c_str(), m_exclusive, i_switch);

    if (id != 0) {
      m_is_OTF = 0;
    }
#ifdef DEBUG_PRINT_OTF
    if (my_rank == 0) {
		fprintf(stderr, "\t<labelOTF> label=%s, m_exclusive=%d, i_switch=%d\n",
			label.c_str(), m_exclusive, i_switch);
    }
#endif
#endif
  }


  /// OTF 出力処理を終了する
  ///
  void PerfWatch::finalizeOTF(void)
  {
#ifdef USE_OTF
    if (m_is_OTF == 0) return;

    std::string s_group, s_counter, s_unit;

	s_group = "PMlib-OTF counter group" ;

    int is_unit = statsSwitch();
	#ifdef DEBUG_PRINT_OTF
    if (my_rank == 0) {
	fprintf(stderr, "\t<finalizeOTF> is_unit=%d \n", is_unit);
	fprintf(stderr, "\tmy_papi.num_sorted-1=%d \n", my_papi.num_sorted-1);
    }
	#endif
	if ( (is_unit == 0) || (is_unit == 1) ) {
		s_counter =  "User Defined COMM/CALC values" ;
		s_unit =  "unit: B/sec or Flops";
	} else if ( (2 <= is_unit) && (is_unit <= 4) ) {
		s_counter =  "HWPC measured values" ;
		s_unit =  my_papi.s_sorted[my_papi.num_sorted-1] ;
	}

	(void) MPI_Barrier(MPI_COMM_WORLD);
	my_otf_finalize (num_process, my_rank, is_unit,
		otf_filename.c_str(), s_group.c_str(),
		s_counter.c_str(), s_unit.c_str());

    m_is_OTF = 0;

	#ifdef DEBUG_PRINT_OTF
    if (my_rank == 0) {
	fprintf(stderr, "\t<finalizeOTF> otf_filename=%s, is_unit=%d, s_unit=%s \n",
		otf_filename.c_str(), is_unit, s_unit.c_str());
    }
	#endif
#endif
  }



  /// 測定区間スタート.
  ///
  void PerfWatch::start()
  {
    if (m_started) {
      printError("start()",  "was already started. Ignored the call.\n");
      return;
    }
    if (m_exclusive && ExclusiveStarted) {
      printError("start()",  "Overlapped the previous exclusive section.\n");
      printError("start()",  "PMlib ignores this section.\n");
      m_is_healthy=false;
      return;
    }
    m_started = true;
    if (m_exclusive) ExclusiveStarted = true;
    m_startTime = getTime();

#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0)
		fprintf (stderr, "<PerfWatch::start> [%s] \n", m_label.c_str());
#endif

#ifdef USE_PAPI

	if (m_is_first) {
		my_papi = papi;
		m_is_first = false;
	}
	for (int i=0; i<my_papi.num_events; i++){
		my_papi.values[i] = 0;
	}

	#pragma omp parallel
	{
		struct pmlib_papi_chooser th_papi = my_papi;
		int i_ret;

		i_ret = my_papi_bind_start (th_papi.values, th_papi.num_events);
		//
		//	Development note:
		//	If we should support HWPC measurement for inclusive sections,
		//	a start()/stop() pair may not be used, because it clears out
		//	the event counters.
		//	To produce the HWPC data for inclusive sections, we should use
		//	my_papi_bind_read() instead of my_papi_bind_start/stop() as below.
		//	In such a case, we will have to modify the my_papi.values[*]
		//	and my_papi.accumu[*] usage.
		//
		//	i_ret = my_papi_bind_read (th_papi.values, th_papi.num_events);

		if ( i_ret != PAPI_OK ) {
			int i_thread = omp_get_thread_num();
			fprintf(stderr, "*** error. <my_papi_bind_start> code: %d, thread:%d\n", i_ret, i_thread);
			PM_Exit(0);
		}

		#ifdef DEBUG_PRINT_PAPI_THREADS
		#pragma omp critical
		{
    		if (my_rank == 0) {
			int i_thread = omp_get_thread_num();
				fprintf (stderr, "\tthread:%d, th_papi.values[*]: ", i_thread);
				for (int i=0; i<my_papi.num_events; i++) {
				fprintf (stderr, "%llu, ", th_papi.values[i]);
				}
				fprintf (stderr, "\n");
			}
		}
		#endif
	}

#endif // USE_PAPI

#ifdef USE_OTF
    if (m_is_OTF != 0) {
      int is_unit = statsSwitch();
      //	if (is_unit != 1) is_unit = 0;
      my_otf_event_start(my_rank, m_startTime, m_id, is_unit);
	}
#endif

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
    if (!(m_is_healthy)) {
      printError("stop()",  "PMlib ignores this section.\n");
      m_is_healthy=false;
      return;
    }
    if (!m_started) {
      printError("PerfWatch::stop()",  "not started\n");
      m_is_healthy=false;
      return;
    }

    m_stopTime = getTime();
    m_time += m_stopTime - m_startTime;
    m_count++;
    m_started = false;
    if (m_exclusive) ExclusiveStarted = false;


#ifdef USE_PAPI
	if (m_exclusive) {
	#pragma omp parallel
	{

		struct pmlib_papi_chooser th_papi = my_papi;
		int i_ret;

		//	i_ret = my_papi_bind_stop (th_papi.values, th_papi.num_events);

		i_ret = my_papi_bind_stop (th_papi.values, th_papi.num_events);

		if ( i_ret != PAPI_OK ) {
			int i_thread = omp_get_thread_num();
			printError("stop()",  "my_papi_bind_stop code: %d, i_thread:%d\n",
								i_ret, i_thread);
		}

		// 2016/6/27 Development note:
		// PAPIでは start()/stop() ペアによりカウンター情報がリセットされる
		// このため非排他区間のHWPC情報を保存するためには my_papi データを
		// 保持する仕組みに修正が必要となる
		//	If we should support HWPC measurement for inclusive sections,
		//	a start()/stop() pair may not be used, because it clears out
		//	the event counters.
		//	We will need to use my_papi_bind_read() instead, as below.
		//	In such a case, we will have to modify the my_papi.values[*]
		//	and my_papi.accumu[*] usage.
		//
		//	i_ret = my_papi_bind_read (th_papi.values, th_papi.num_events);


		#pragma omp critical
		{
			for (int i=0; i<my_papi.num_events; i++) {
			my_papi.values[i] += th_papi.values[i];
			}
		}

		#ifdef DEBUG_PRINT_PAPI_THREADS
		#pragma omp critical
		{
    		if (my_rank == 0) {
			int i_thread = omp_get_thread_num();
				fprintf (stderr, "\tthread:%d, th_papi.values[*]: ", i_thread);
				for (int i=0; i<my_papi.num_events; i++) {
				fprintf (stderr, "%llu, ", th_papi.values[i]);
				}
				fprintf (stderr, "\n");
			}
		}
		#endif
	}

	} else { //	!(m_exclusive)
		//	If inclusive, do not collect HWPC data
	}


	for (int i=0; i<my_papi.num_events; i++) {
		my_papi.accumu[i] += my_papi.values[i];
	}

	#ifdef DEBUG_PRINT_PAPI
    if (my_rank == 0) {
		int n_thread = omp_get_max_threads();
		fprintf (stderr, "  my_papi : num_events=%d, sum of %d threads \n",
				my_papi.num_events, n_thread);
		for (int i=0; i<my_papi.num_events; i++) {
			fprintf (stderr, "  event i=%d, value=%llu, accumu=%llu\n",
				i, my_papi.values[i], my_papi.accumu[i]);
		}
	}
	#endif

#endif


    int is_unit = statsSwitch();
	if ( (is_unit == 0) || (is_unit == 1) ) {
		// ユーザが引数で指定した計算量
		m_flop += flopPerTask * (double)iterationCount;
	}

#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) {
		fprintf (stderr, "<PerfWatch::stop> [%s] fPT=%e, itC=%u, m_time=%f, m_flop=%e\n"
			, m_label.c_str(), flopPerTask, iterationCount, m_time, m_flop);
    }
#endif



#ifdef USE_OTF
	double w=0.0;

	if (m_is_OTF == 0) {
		// OTFファイル出力なし
		;
	} else if (m_is_OTF == 1) {
		// OTFファイルには時間情報だけを出力し、カウンター値は0.0とする
		w = 0.0;
		my_otf_event_stop(my_rank, m_stopTime, m_id, is_unit, w);

	} else if (m_is_OTF == 2) {
		if ( (is_unit == 0) || (is_unit == 1) ) {
			// ユーザが引数で指定した計算量/time(計算speed)
    		//	w = (flopPerTask * (double)iterationCount) / m_time;
    		w = (flopPerTask * (double)iterationCount) / (m_stopTime-m_startTime);
		} else if ( (2 <= is_unit) && (is_unit <= 4) ) {
			// 自動計測されたHWPCイベントを分析した計算speed
			sortPapiCounterList ();
			w = my_papi.v_sorted[my_papi.num_sorted-1] ;
		}
		my_otf_event_stop(my_rank, m_stopTime, m_id, is_unit, w);
	}

#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) {
		fprintf (stderr, "\t[%s] w=%e, m_time=%f, m_flop=%e \n"
				, m_label.c_str(), w, m_time, m_flop );
    }
#endif
#endif

  }

} /* namespace pm_lib */
