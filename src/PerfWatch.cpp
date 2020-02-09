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

//! @file   PerfWatch.cpp
//! @brief  PerfWatch class

// When compiling with USE_PAPI macro, openmp option should be enabled.

#ifdef DISABLE_MPI
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

extern void sortPapiCounterList ();
extern void outputPapiCounterHeader (FILE*, std::string);
extern void outputPapiCounterList (FILE*);
extern void outputPapiCounterLegend (FILE*);


namespace pm_lib {

  struct pmlib_papi_chooser papi;
  struct hwpc_group_chooser hwpc_group;
  double cpu_clock_freq;        /// processor clock frequency, i.e. Hz
  double second_per_cycle;  /// real time to take each cycle

  //	bool PerfWatch::ExclusiveStarted = false;

  /// 単位変換.
  ///
  ///   @param[in] fops 浮動小数点演算量又はデータ移動量
  ///   @param[out] unit 単位の文字列
  ///   @param[in] is_unit ユーザー申告値かHWPC自動測定値かの指定
  ///              = 0: ユーザが引数で指定したデータ移動量(バイト)
  ///              = 1: ユーザが引数で指定した演算量(浮動小数点演算量)
  ///              = 2: HWPC が自動測定する data access bandwidth event
  ///              = 3: HWPC が自動測定する flops event
  ///              = 4: HWPC が自動測定する vectorization (SSE, AVX, SVE, etc)
  ///              = 5: HWPC が自動測定する cache hit, miss,
  ///              = 6: HWPC が自動測定する cycles, instructions
  ///              = 7: HWPC が自動測定する load/store instruction type
  ///   @return  単位変換後の数値
  ///
  ///   @note is_unitは通常PerfWatch::statsSwitch()で事前に決定されている
  ///

  double PerfWatch::unitFlop(double fops, std::string &unit, int is_unit)
  {

    double P, T, G, M, K, ret=0.0;
    K = 1000.0;
    M = 1000.0*K;
    G = 1000.0*M;
    T = 1000.0*G;
    P = 1000.0*T;

    if ( (is_unit == 0) || (is_unit == 2) )  {
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
    } else

    if ( (is_unit == 1) || (is_unit == 3) )  {
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
    } else

    if ( (is_unit == 4) || (is_unit == 5) || (is_unit == 7) )  {
        ret = fops;
        unit = "(%)";
    } else

    if ( is_unit == 6 )  {
      if      ( fops > P ) {
        ret = fops / P;
        unit = "P.ips";
      }
      else if ( fops > T ) {
        ret = fops / T;
        unit = "T.ips";
      }
      else if ( fops > G ) {
        ret = fops / G;
        unit = "G.ips";
      }
      else {
        ret = fops / M;
        unit = "M.ips";
      }
    }

    return ret;
  }



  /// HWPCによるイベントカウンターの測定値を Allgather する
  ///
  ///
  void PerfWatch::gatherHWPC()
  {
#ifdef USE_PAPI
	int is_unit = statsSwitch();
	if ( (is_unit == 0) || (is_unit == 1) ) {
		return;
	}
	//	if ( my_papi.num_events == 0) return;

	sortPapiCounterList ();

    // 0: user set bandwidth
    // 1: user set flop counts
    // 2: BANDWIDTH : HWPC measured data access bandwidth
    // 3: FLOPS     : HWPC measured flop counts
    // 4: VECTOR    : HWPC measured vectorization
    // 5: CACHE     : HWPC measured cache hit/miss
    // 6: CYCLE     : HWPC measured cycles, instructions
    // 7: LOADSTORE : HWPC measured load/store instruction type
	m_flop = 0.0;
	m_percentage = 0.0;
	if ( is_unit >= 0 && is_unit <= 1 ) {
		m_flop = m_time * my_papi.v_sorted[my_papi.num_sorted-1] ;
	} else 
	if ( is_unit == 2 ) {
		m_flop = my_papi.v_sorted[my_papi.num_sorted-2] ;		// BYTES
	} else 
	if ( is_unit == 3 ) {
		m_flop = my_papi.v_sorted[my_papi.num_sorted-3] ;		// Total_FP
	} else 
	if ( is_unit == 4 ) {
		m_flop = my_papi.v_sorted[my_papi.num_sorted-3] ;		// Total_FP
		m_percentage = my_papi.v_sorted[my_papi.num_sorted-1] ;	// [Vector %]
	} else 
	if ( is_unit == 5 ) {
		//	m_flop = my_papi.v_sorted[0] ;							// load+store K/FX100
		m_flop = my_papi.v_sorted[0] + my_papi.v_sorted[1] ;	// load+store
		m_percentage = my_papi.v_sorted[my_papi.num_sorted-1] ;	// [L1L2hit%]
	} else
	if ( is_unit == 6 ) {
		m_flop = my_papi.v_sorted[my_papi.num_sorted-2] ;		// TOT_INS
	} else
	if ( is_unit == 7 ) {
		//	m_flop = my_papi.v_sorted[0] ;							// load+store K/FX100
		m_flop = my_papi.v_sorted[0] + my_papi.v_sorted[1] ;	// load+store
		m_percentage = my_papi.v_sorted[my_papi.num_sorted-1] ;	// [Vector %]
	}

	// The space is reserved only once as a fixed size array
	if ( m_sortedArrayHWPC == NULL) {
		m_sortedArrayHWPC = new double[num_process*my_papi.num_sorted];
		if (!(m_sortedArrayHWPC)) {
			printError("gatherHWPC", "new memory failed. %d x %d x 8\n", num_process, my_papi.num_sorted);
			PM_Exit(0);
		}
	}

	if ( num_process > 1 ) {
		int iret =
		MPI_Allgather (my_papi.v_sorted, my_papi.num_sorted, MPI_DOUBLE,
					m_sortedArrayHWPC, my_papi.num_sorted, MPI_DOUBLE, MPI_COMM_WORLD);
		if ( iret != 0 ) {
			printError("gatherHWPC", " MPI_Allather failed.\n");
			PM_Exit(0);
		}
	} else {

        for (int i = 0; i < my_papi.num_sorted; i++) {
			m_sortedArrayHWPC[i] = my_papi.v_sorted[i];
		}
	}

	//	#ifdef DEBUG_PRINT_PAPI_THREADS
	//	fprintf(stderr, "\t<PerfWatch::gatherHWPC> finishing [%15s]  my_rank=%d, m_count=%d\n", m_label.c_str(), my_rank, m_count);
	//	#endif

#endif
  }


  /// 測定結果情報をノード０に集約.
  ///
  void PerfWatch::gather()
  {

    int m_np;
    m_np = num_process;

	// The space should be reserved only once as fixed size arrays
	if (( m_timeArray == NULL) && ( m_flopArray == NULL) && ( m_countArray == NULL)) {
		m_timeArray  = new double[m_np];
		m_flopArray  = new double[m_np];
		m_countArray  = new long[m_np];
		if (!(m_timeArray) || !(m_flopArray) || !(m_countArray)) {
			printError("PerfWatch::gather", "new memory failed. %d(process) x 3 x 8 \n", num_process);
			PM_Exit(0);
		}

		#ifdef DEBUG_PRINT_WATCH
		fprintf(stderr, "\t<PerfWatch::gather> [%15s] my_rank=%d. Allocated new m_countArray[%d] at address:%p and others.\n",
			m_label.c_str(), my_rank, m_np, m_countArray);
		#endif
	}

    if ( m_np == 1 ) {
      m_timeArray[0] = m_time;
      m_flopArray[0] = m_flop;
      m_countArray[0]= m_count;
      m_count_sum = m_count;
    } else {
      if (MPI_Allgather(&m_time,  1, MPI_DOUBLE, m_timeArray, 1, MPI_DOUBLE, MPI_COMM_WORLD) != MPI_SUCCESS) PM_Exit(0);
      if (MPI_Allgather(&m_flop,  1, MPI_DOUBLE, m_flopArray, 1, MPI_DOUBLE, MPI_COMM_WORLD) != MPI_SUCCESS) PM_Exit(0);
      if (MPI_Allgather(&m_count, 1, MPI_LONG, m_countArray, 1, MPI_LONG, MPI_COMM_WORLD) != MPI_SUCCESS) PM_Exit(0);
      if (MPI_Allreduce(&m_count, &m_count_sum, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD) != MPI_SUCCESS) PM_Exit(0);
    }
	// Above arrays will be used by the subsequent routines, and should not be deleted here
	// i.e. m_timeArray, m_flopArray, m_countArray
	#ifdef DEBUG_PRINT_WATCH
	(void) MPI_Barrier(MPI_COMM_WORLD);
	if (my_rank == 0) {
		fprintf(stderr, "\t<PerfWatch::gather> [%15s] m_countArray[0:*]:", m_label.c_str() );
		for (int i=0; i<num_process; i++) { fprintf(stderr, " %d",  m_countArray[i]); } fprintf(stderr, "\n");
	}
	(void) MPI_Barrier(MPI_COMM_WORLD);
	#endif
  }


  /// Statistics among processes
  /// Translate in Japanese later on...
  /// 全プロセスの測定結果の平均値・標準偏差などの基礎的な統計計算
  /// 測定区間の呼び出し回数はプロセス毎に異なる場合がありえる
  ///
  void PerfWatch::statsAverage()
  {
	//	if (my_rank != 0) return;	// This was a bad idea. All ranks should compute the stats.

	// 平均値
	m_time_av = 0.0;
	m_flop_av = 0.0;
	for (int i = 0; i < num_process; i++) {
		m_time_av += m_timeArray[i];
		m_flop_av += m_flopArray[i];
	}
	m_time_av /= num_process;
	m_flop_av /= num_process;

	if (m_in_parallel) {
		m_count_av = lround((double)m_count_sum / (double)num_process);
	} else {
		m_count_av = lround((double)m_count_sum / (double)num_process);
	}
	
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
  }


  /// 計算量の選択を行う
  ///
  /// @return
  ///   0: ユーザが引数で指定したデータ移動量(バイト)を採用する
  ///   1: ユーザが引数で指定した計算量を採用する "Flops"
  ///   2: HWPC が自動的に測定する data access bandwidth
  ///   3: HWPC が自動的に測定する flops event
  ///   4: HWPC が自動的に測定する vectorized f.p. (SSE, AVX, SVE, etc) event
  ///   5: HWPC が自動的に測定する cache hit, miss
  ///   6: HWPC が自動的に測定する cycles, instructions
  ///   7: HWPC が自動的に測定する load/store instruction type
  ///
  /// @note
  /// 計算量としてユーザー申告値を用いるかHWPC計測値を用いるかの決定を行う
  /// 環境変数HWPC_CHOOSERの値が優先される
  ///
  int PerfWatch::statsSwitch()
  {
    int is_unit;

    // 0: user set bandwidth
    // 1: user set flop counts
    // 2: BANDWIDTH : HWPC measured data access bandwidth
    // 3: FLOPS     : HWPC measured flop counts
    // 4: VECTOR    : HWPC measured vectorization
    // 5: CACHE     : HWPC measured cache hit/miss
    // 6: CYCLE     : HWPC measured cycles, instructions
    // 7: LOADSTORE : HWPC measured load/store instruction type

    if (hwpc_group.number[I_bandwidth] > 0) {
      is_unit=2;
    } else if (hwpc_group.number[I_flops]  > 0) {
      is_unit=3;
    } else if (hwpc_group.number[I_vector] > 0) {
      is_unit=4;
    } else if (hwpc_group.number[I_cache] > 0) {
      is_unit=5;
    } else if (hwpc_group.number[I_cycle] > 0) {
      is_unit=6;
    } else if (hwpc_group.number[I_loadstore] > 0) {
      is_unit=7;
    } else if (m_typeCalc == 0) {
		is_unit=0;
    } else if (m_typeCalc == 1) {
		is_unit=1;
    } else {
    	is_unit=-1;	// this is an error case...
	}

    return is_unit;
  }


#if defined (USE_PRECISE_TIMER) // Platform specific precise timer
	#if defined (__APPLE__)				// Mac Clang and/or GCC
		#include <unistd.h>
		#include <mach/mach.h>
		#include <mach/mach_time.h>
	#elif defined (__FUJITSU)			// Fugaku A64FX, FX100, K computer
		#include <fjcex.h>
	#endif
#endif

  /// 時刻を取得
  ///
  ///   @return 時刻値(秒)
  ///
  double PerfWatch::getTime()
  {
#if defined (USE_PRECISE_TIMER) // Platform specific precise timer
	#if defined (__APPLE__)				// Mac Clang and/or GCC
		//	printf("[__APPLE__] is defined.\n");
		//	return ((double)mach_absolute_time() * second_per_cycle);
		// mach_absolute_time() appears to return nano-second unit value
		return ((double)mach_absolute_time() * 1.0e-9);

	#elif defined (__FUJITSU)			// Fugaku A64FX, FX100, K computer and Fujitsu compiler/library
		//	printf("[__FUJITSU] is defined.\n");
		register double tval;
		tval = __gettod()*1.0e-6;
		return (tval);

	#elif defined(__x86_64__)			// Intel Xeon processor
		#if defined (__INTEL_COMPILER) || (__gnu_linux__)
		// Can replace the following code segment using inline assembler
		unsigned long long tsc;
		unsigned int lo, hi;
		__asm __volatile__ ( "rdtsc" : "=a"(lo), "=d"(hi) );
		tsc = ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
		return ((double)tsc * second_per_cycle);

		#else	// precise timer is not available. use gettimeofday() instead.
		struct timeval tv;
		gettimeofday(&tv, 0);
		return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
		#endif
	#else		// precise timer is not available. use gettimeofday() instead.
		struct timeval tv;
		gettimeofday(&tv, 0);
		return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
	#endif
#else // Portable timer gettimeofday() on Linux, Unix, Macos
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
#endif

  }


  void PerfWatch::read_cpu_clock_freq()
  {
	cpu_clock_freq = 1.0;
	second_per_cycle = 1.0;
#if defined (USE_PRECISE_TIMER)
	#if defined (__APPLE__)
		FILE *fp;
		char buf[256];
		long long llvalue;
		//	fp = popen("sysctl -n machdep.cpu.brand_string", "r");
		//		string "Intel(R) Core(TM) i7-6820HQ CPU @ 2.70GHz"
		fp = popen("sysctl -n hw.cpufrequency", "r");
		//		integer 2700000000
		if (fp == NULL) {
			printError("<read_cpu_clock_freq>",  "popen(sysctl) failed. \n");
			return;
		}
		if (fgets(buf, sizeof(buf), fp) != NULL) {
			//	fprintf(stderr, "<read_cpu_clock_freq> buf=%s \n", buf);
			sscanf(buf, "%lld", &llvalue);
			if (llvalue <= 0) {
				printError("<read_cpu_clock_freq>",  "invalid hw.cpufrequency \n");
				llvalue=1.0;
			}
			cpu_clock_freq = (double)llvalue;
			second_per_cycle = 1.0/cpu_clock_freq;
			#ifdef DEBUG_PRINT_WATCH
			if (my_rank == 0) {
				fprintf(stderr, "<read_cpu_clock_freq> cpu_clock_freq=%lf second_per_cycle=%16.12lf \n",
									cpu_clock_freq, second_per_cycle);
			}
			#endif
		} else {
			printError("<read_cpu_clock_freq>",  "can not detect hw.cpufrequency \n");
		}
		pclose(fp);
		return;

	#elif defined (__FUJITSU)			// Fugaku A64FX, FX100, K computer and Fujitsu compiler/library
		//	__gettod() on Fujitsu compiler/library doesn't require cpu_clock_freq
		return;

	#elif defined(__x86_64__)					// Intel Xeon
		#if defined (__INTEL_COMPILER) || (__gnu_linux__)
	    // read the cpu freqency from /proc/cpuinfo

	    FILE *fp;
	    double value;
	    char buffer[1024];
	    fp = fopen("/proc/cpuinfo","r");
	    if (fp == NULL) {
	    	printError("<read_cpu_clock_freq>",  "Can not open /proc/cpuinfo \n");
	    	return;
	    }
	    while (fgets(buffer, 1024, fp) != NULL) {
			//	fprintf(stderr, "<read_cpu_clock_freq> buffer=%s \n", buffer);
	    	if (!strncmp(buffer, "cpu MHz",7)) {
	    		sscanf(buffer, "cpu MHz\t\t: %lf", &value);
	    		// sscanf handles regexp such as: sscanf (buffer, "%[^\t:]", value);
	    		cpu_clock_freq = (value * 1.0e6);
	    		break;
	    	}
	    }
	    fclose(fp);
	    if (cpu_clock_freq <= 0.0) {
	    	printError("<read_cpu_clock_freq>",  "Failed parsing /proc/cpuinfo \n");
	    	return;
	    }
	    second_per_cycle = 1.0/(double)cpu_clock_freq;
		#ifdef DEBUG_PRINT_WATCH
	   	if (my_rank == 0 && my_thread == 0) {
			fprintf(stderr, "<read_cpu_clock_freq> cpu_clock_freq=%lf second_per_cycle=%16.12lf \n",
								cpu_clock_freq, second_per_cycle);
	   	 }
		#endif

		#else
		return;
	    #endif
	#endif
#endif	// (USE_PRECISE_TIMER)
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
    if (is_unit == 1) unit = "Flops";	// 1: user set flop counts
	//
    if (is_unit == 2) unit = "";		// 2: HWPC measured bandwidth
    if (is_unit == 3) unit = "";		// 3: HWPC measured flop counts
    if (is_unit == 4) unit = "";		// 4: HWPC measured vector %
    if (is_unit == 5) unit = "";		// 5: HWPC measured cache hit%
    if (is_unit == 6) unit = "";		// 6: HWPC measured instructions
    if (is_unit == 7) unit = "";		// 7: HWPC measured memory load/store (demand access, prefetch, writeback, streaming store)

    long total_count = 0;
    for (int i = 0; i < m_np; i++) total_count += m_countArray[i];

    if ( total_count > 0 && is_unit <= 1) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
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
    } else if ( total_count > 0 && is_unit >= 2) {
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
    if (is_unit == 1) unit = "Flops";	// 1: user set flop counts
	//
    if (is_unit == 2) unit = "";		// 2: HWPC measured bandwidth
    if (is_unit == 3) unit = "";		// 3: HWPC measured flop counts
    if (is_unit == 4) unit = "";		// 4: HWPC measured vector %
    if (is_unit == 5) unit = "";		// 5: HWPC measured cache hit%
    if (is_unit == 6) unit = "";		// 6: HWPC measured instructions
    if (is_unit == 7) unit = "";		// 7: HWPC measured memory load/store (demand access, prefetch, writeback, streaming store)



    long total_count = 0;
    for (int i = 0; i < m_np; i++) total_count += m_countArray[pp_ranks[i]];

    if ( total_count > 0 && is_unit <= 1) {
      fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
      fprintf(fp, "Header ID  :     call   time[s] time[%%]  t_wait[s]  t[s]/call   operations  performance\n");
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
    } else if ( total_count > 0 && is_unit >= 2) {
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
    //	char* c_env = std::getenv("HWPC_CHOOSER");
    //	if (c_env == NULL) return;
    if (my_papi.num_events == 0) return;
    if (!m_exclusive) return;
    if ( m_count_sum == 0 ) return;
    if (my_rank == 0) {
      outputPapiCounterHeader (fp, s_label);
      outputPapiCounterList (fp);
    }
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
    //	char* c_env = std::getenv("HWPC_CHOOSER");
    //	if (c_env == NULL) return;
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
   char* c_env;
	std::string s;
#ifdef USE_PAPI
	const PAPI_hw_info_t *hwinfo = NULL;
	using namespace std;

	c_env = std::getenv("HWPC_CHOOSER");
	if (c_env == NULL) {
		fprintf(fp, "\tHWPC_CHOOSER is not set. User API values are reported.\n");
	} else {
		s = c_env;
		if  (s == "FLOPS" || s == "BANDWIDTH" || s == "VECTOR" || s == "CACHE" || s == "CYCLE" || s == "LOADSTORE" ) { 
			fprintf(fp, "\tHWPC_CHOOSER=%s environment variable is provided.\n", s.c_str());
		} else {
			fprintf(fp, "\tUnknown group HWPC_CHOOSER=%s is ignored. User API values are reported.\n", s.c_str());
		}
	}

#endif

#ifdef USE_OTF
    c_env = std::getenv("OTF_TRACING");
    if (c_env != NULL) {
	  fprintf(fp, "\tOTF_TRACING=%s environment variable is provided.\n", c_env);
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
	//	char* c_env = std::getenv("HWPC_CHOOSER");
	//	if (c_env == NULL) return;

	outputPapiCounterLegend (fp);
#endif
  }


  ///  OpenMP並列処理されたPMlibスレッド測定区間のうち parallel regionから
  ///  呼び出された測定区間のスレッド測定情報をマスタースレッドに集約する。
  ///
  ///   @note  内部で全測定区間をcheckして該当する測定区間を選択する。
  ///
  void PerfWatch::mergeAllThreads(void)
  {
  #ifdef _OPENMP
	if (m_threads_merged) return;
	if (m_started) return;	// still active in the middle of start/stop pair

	// If the application calls PMlib from inside the parallel region, the PerfMonitor class must be
	// instantiated as thread private OpenMP construct to preserve thread private my_papi.* memory storage, 
	// and mergeAllThreads() must be called at the end of such parallel region.

	bool is_merger_parallel = omp_in_parallel();
	if (m_in_parallel && !is_merger_parallel) return;

    int is_unit = statsSwitch();
	if ( is_unit >= 2) { // PMlib HWPC counter mode
		if (m_in_parallel) { // this section was executed inside parallel region
			// shared structure "papi" is used temporarily here.  Cleane up "papi" after the use.
			for (int i=0; i<my_papi.num_events; i++) {
				papi.th_accumu[my_thread][i] = my_papi.th_accumu[my_thread][i];
				papi.th_v_sorted[my_thread][i] = my_papi.th_v_sorted[my_thread][i];
			}
			#pragma omp barrier
			for (int j=0; j<num_threads; j++) {
			for (int i=0; i<my_papi.num_events; i++) {
				my_papi.th_accumu[j][i] = papi.th_accumu[j][i] ;
				my_papi.th_v_sorted[j][i] = papi.th_v_sorted[j][i] ;
			}
			}
			#pragma omp barrier
			for (int i=0; i<my_papi.num_events; i++) {
				papi.th_accumu[my_thread][i] = 0;
				papi.th_v_sorted[my_thread][i] = 0.0;
			}
		} // end of if (m_in_parallel)

		for (int i=0; i<my_papi.num_events; i++) {
			my_papi.accumu[i] = 0.0;
			for (int j=0; j<num_threads; j++) {
			my_papi.accumu[i] += my_papi.th_accumu[j][i];
			}
		}

	} else {	// PMlib user counter mode
		if (m_in_parallel) { // this section was executed inside parallel region
			//  PerfWatch::stop() should have saved following variables
			//	my_papi.th_v_sorted[my_thread][0] = (double)m_count;	// call
			//	my_papi.th_v_sorted[my_thread][1] = m_time;				// time[s]
			//	my_papi.th_v_sorted[my_thread][2] = m_flop;				// operations
			for (int i=0; i<3; i++) {
				papi.th_v_sorted[my_thread][i] = my_papi.th_v_sorted[my_thread][i];
			}
			#pragma omp barrier
			for (int j=0; j<num_threads; j++) {
			for (int i=0; i<3; i++) {
				my_papi.th_v_sorted[j][i] = papi.th_v_sorted[j][i] ;
			}
			}
			#pragma omp barrier
			for (int i=0; i<3; i++) {
				papi.th_v_sorted[my_thread][i] = 0.0;
			}
		}
	}

	if (m_in_parallel) {
	double m_count_threads, m_time_threads, m_flop_threads;
	#pragma omp barrier
		m_count_threads = 0.0;
		m_time_threads  = 0.0;
		m_flop_threads  = 0.0;
		for (int j=0; j<num_threads; j++) {
			//	m_count_threads += my_papi.th_v_sorted[j][0];
			//	m_time_threads += my_papi.th_v_sorted[j][1];
			m_count_threads = std::max(m_count_threads, my_papi.th_v_sorted[j][0]);
			m_time_threads = std::max(m_time_threads, my_papi.th_v_sorted[j][1]);
			m_flop_threads += my_papi.th_v_sorted[j][2];
		}
		//	m_count = lround(m_count_threads/(double)num_threads);
		m_count = lround(m_count_threads);
		m_time = m_time_threads;
		m_flop = m_flop_threads;
	} else {
		// In Worksharing parallel structure, everything is in place.
		;
	}

	m_threads_merged = true;


	#ifdef DEBUG_PRINT_PAPI_THREADS
    if (my_rank == 0) {
	#pragma omp barrier
	#pragma omp critical
	{
    	fprintf(stderr, "<mergeAllThreads> [%s] my_thread=%d m_in_parallel=%s, is_merger_parallel=%s \n",
			m_label.c_str(), my_thread, m_in_parallel?"true":"false", is_merger_parallel?"true":"false");
		if ( is_unit >= 2) { // PMlib HWPC counter mode
    		fprintf(stderr, "\t<mergeAllThreads> [%s] my_thread=%d\n", m_label.c_str(), my_thread);
			for (int i=0; i<my_papi.num_events; i++) {
				fprintf(stderr, "\t [%s] my_thread=%d  printing: [%8s] my_papi.accumu[%d]=%llu \n",
					m_label.c_str(), my_thread, my_papi.s_name[i].c_str(), i, my_papi.accumu[i]);
				for (int j=0; j<num_threads; j++) {
					fprintf(stderr, "\t\t my_papi.th_accumu[%d][%d]=%llu\n", j, i, my_papi.th_accumu[j][i]);
				}
			}
		} else {	// ( is_unit == 0 | is_unit == 1) : PMlib user counter mode
    		fprintf(stderr, "\t<mergeAllThreads> [%s] user mode: my_thread=%d, m_flop=%e\n", m_label.c_str(), my_thread, m_flop);
			for (int j=0; j<num_threads; j++) {
				fprintf (stderr, "\t my_papi.th_v_sorted[%d][0:2]: %e, %e, %e \n",
					j, my_papi.th_v_sorted[j][0], my_papi.th_v_sorted[j][1], my_papi.th_v_sorted[j][2]);
			}
		}
		if (m_in_parallel) {
			fprintf (stderr, "\t m_count=%d, m_time=%e, m_flop=%e\n", m_count, m_time, m_flop);
		}
	}
    }
	#endif

  #endif
  }


  void PerfWatch::selectPerfSingleThread(int i_thread)
  {
	for (int ip=0; ip<my_papi.num_events; ip++) {
		my_papi.accumu[ip] = my_papi.th_accumu[i_thread][ip];
	}

    	//	int is_unit = statsSwitch();
		//	if (is_unit < 2 && !m_in_parallel) {
	if (m_in_parallel) {
		m_count = llround(my_papi.th_v_sorted[i_thread][0]);
		m_time = my_papi.th_v_sorted[i_thread][1];
		m_flop = my_papi.th_v_sorted[i_thread][2];
	} else {
		m_count = llround(my_papi.th_v_sorted[0][0]);
		m_time = my_papi.th_v_sorted[0][1];
		m_flop = my_papi.th_v_sorted[0][2];
	}

#ifdef DEBUG_PRINT_PAPI_THREADS
    if (my_rank == 0) {
    	fprintf(stderr, "\t <selectPerfSingleThread> [%s] my_rank=%d, i_thread=%d, m_time=%e, m_flop=%e, m_count=%lu\n", m_label.c_str(), my_rank, i_thread, m_time, m_flop, m_count );
    }
#endif
  }


  /// スレッド別詳細レポートを出力。
  ///
  ///   @param[in] fp           出力ファイルポインタ
  ///   @param[in] rank_ID      出力対象プロセスのランク番号
  ///
  void PerfWatch::printDetailThreads(FILE* fp, int rank_ID)
  {
    double perf_rate;
	if(rank_ID<0 || rank_ID>num_process) return;

    std::string unit;
    int is_unit = statsSwitch();

    if (is_unit == 0) unit = "B/sec";		// 0: user set bandwidth
    if (is_unit == 1) unit = "Flops";		// 1: user set flop counts
	//
    if (is_unit == 2) unit = "";		// 2: HWPC measured bandwidth
    if (is_unit == 3) unit = "";		// 3: HWPC measured flop counts
    if (is_unit == 4) unit = "";		// 4: HWPC measured vector %
    if (is_unit == 5) unit = "";		// 5: HWPC measured cache hit%
    if (is_unit == 6) unit = "";		// 6: HWPC measured instructions
    if (is_unit == 7) unit = "";		// 7: HWPC measured memory load/store (demand access, prefetch, writeback, streaming store)

	if (my_rank == 0 && is_unit < 2) {
	    fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());
    	fprintf(fp, "Thread  call  time[s]  t/tav[%%]  operations  performance\n");
	} else 
	if (my_rank == 0 && is_unit >= 2) {
	    fprintf(fp, "Label  %s%s\n", m_exclusive ? "" : "*", m_label.c_str());

		std::string s;
		int ip, jp, kp;
    	fprintf(fp, "Thread  call  time[s]  t/tav[%%]");
		for(int i=0; i<my_papi.num_sorted; i++) {
			kp = my_papi.s_sorted[i].find_last_of(':');
			if ( kp < 0) {
				s = my_papi.s_sorted[i];
			} else {
				s = my_papi.s_sorted[i].substr(kp+1);
			}
			fprintf (fp, " %10.10s", s.c_str() );
		} fprintf (fp, "\n");
	}

	//	The following health check does not work well with MPI...
	//	if (my_rank == rank_ID && !m_threads_merged)  
	//		fprintf(fp, "\t thread performance statistics is not available.\n");

	int i=rank_ID;

	// Well, we are going to destroy the process based stats with thread based stat.
	// Let's save some of them for later re-use.
	long save_m_count;
	double save_m_time, save_m_flop, save_m_time_av;
	save_m_count = m_count;
	save_m_time  = m_time;
	save_m_flop  = m_flop;
	save_m_time_av  = m_time_av;

	for (int j=0; j<num_threads; j++)
	{
		if ( !m_in_parallel && is_unit < 2 ) {

			if (j == 1) {
				if (my_rank == 0) {
				// user mode thread statistics for worksharing construct
				// are always represented by thread 0, because they can not be
				// split into threads in artificial manner.
				fprintf(fp, " %3d\t\t user mode worksharing threads are represented by thread 0\n", j);
				}
				continue;
			}
			if (j >= 1) {
				if (my_rank == 0) {
				fprintf(fp, " %3d\t\t ditto\n", j);
				}
				continue;
			}
		}

		PerfWatch::selectPerfSingleThread(j);

		PerfWatch::gatherHWPC();

		PerfWatch::gather();

		if (my_rank == 0) {
			if (is_unit < 2) {
			perf_rate = (m_countArray[i]==0) ? 0.0 : m_flopArray[i]/m_timeArray[i];
			fprintf(fp, " %3d%8ld  %9.3e  %5.1f   %9.3e  %9.3e %s\n",
				j,
				m_countArray[i], // コール回数
				m_timeArray[i],  // 時間
				100*m_timeArray[i]/m_time_av, // 時間の比率
				m_flopArray[i],  // 演算数
				perf_rate,       // スピード　Bytes/sec or Flops
				unit.c_str()     // スピードの単位
				);
				(void) fflush(fp);
			}
			else 
			if (is_unit >= 2) {
			fprintf(fp, " %3d%8ld  %9.3e  %5.1f ",
				j,
				m_countArray[i], // コール回数
				m_timeArray[i],  // 時間
				100*m_timeArray[i]/m_time_av);
	
				for(int n=0; n<my_papi.num_sorted; n++) {
				fprintf (fp, "  %9.3e", fabs(m_sortedArrayHWPC[i*my_papi.num_sorted + n]));
				}
				fprintf (fp, "\n");
				(void) fflush(fp);
			}
		}	// end of if (my_rank == 0) 
		#pragma omp barrier
	}	// end of for (int j=0; j<num_threads; j++)
	m_count = save_m_count;
	m_time  = save_m_time;
	m_flop  = save_m_flop;
	m_time_av  = save_m_time_av;
  }



  /// エラーメッセージ出力.
  ///
  ///   @param[in] func メソッド名
  ///   @param[in] fmt  出力フォーマット文字列
  ///
  void PerfWatch::printError(const char* func, const char* fmt, ...)
  {
    if (my_rank == 0) {
      fprintf(stderr, "*** PMlib Error. PerfWatch::%s [%s] ",
                      func, m_label.c_str());
      va_list ap;
      va_start(ap, fmt);
      vfprintf(stderr, fmt, ap);
      va_end(ap);
    }
  }


  /// 測定区間にプロパティを設定.
  ///
  ///   @param[in] label     ラベル
  ///   @param[in] id       ラベルに対応する番号
  ///   @param[in] typeCalc  測定量のタイプ(0:通信, 1:計算)
  ///   @param[in] nPEs            並列プロセス数
  ///   @param[in] my_rank_ID      ランク番号
  ///   @param[in] nTHREADs     並列スレッド数
  ///   @param[in] exclusive 排他測定フラグ
  ///
  void PerfWatch::setProperties(const std::string& label, int id, int typeCalc, int nPEs, int my_rank_ID, int nTHREADs, bool exclusive)
  {
    m_label = label;
    m_id = id;
    m_typeCalc = typeCalc;
    m_exclusive =  exclusive;
    num_process = nPEs;
    my_rank = my_rank_ID;
    num_threads = nTHREADs;
	m_in_parallel = false;
	my_thread = 0;
	m_threads_merged = true;
#ifdef _OPENMP
	m_in_parallel = omp_in_parallel();
	my_thread = omp_get_thread_num();
	m_threads_merged = false;
#endif

	if (!m_is_set) {
		my_papi = papi;
		m_is_set = true;
	}

	if (m_in_parallel) {
	#if defined (__INTEL_COMPILER) || (__PGI)
		// Go ahead.
	#else
		// Nop. This compiler does not support threadprivate C++ class.
		printError("setProperties", "section is not measured. This compiler does not support threadprivate C++ class.\n");
		m_is_set = false;
	#endif
	}


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
      #ifdef DEBUG_PRINT_OTF
      if (my_rank == 0) {
	    fprintf(stderr, "\t<getenv> OTF_TRACING=%s is provided.\n", c_env);
      }
      #endif
    }
#endif

#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) {
    fprintf(stderr, "<PerfWatch::setProperties> [%s] id=%d, typeCalc=%d, nPEs=%d, my_rank_ID=%d, num_threads=%d, exclusive=%s, m_in_parallel=%s\n",
	label.c_str(), id, typeCalc, nPEs, my_rank_ID, num_threads, exclusive?"true":"false", m_in_parallel?"true":"false");
	#ifdef DEBUG_PRINT_PAPI_THREADS
		#pragma omp barrier
		#pragma omp critical
		{
    		fprintf(stderr, "<PerfWatch::setProperties> [%s] my_thread=%d, &(papi)=%p, &(my_papi)=%p\n",
				label.c_str(), my_thread, &papi.num_events, &my_papi.num_events);
			for (int j=0; j<num_threads; j++) {
				fprintf (stderr, "\tmy_papi.th_accumu[%d][*]:", j);
				for (int i=0; i<my_papi.num_events; i++) {
					fprintf (stderr, "%llu, ", my_papi.th_accumu[j][i]);
				};	fprintf (stderr, "\n");
			}
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
	if ( is_unit == 0 || is_unit == 1 ) {
		s_counter =  "User Defined COMM/CALC values" ;
		s_unit =  "unit: B/sec or Flops";
	} else if ( 2 <= is_unit && is_unit <= Max_hwpc_output_group ) {
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
    if (!m_is_healthy) return;

    if (m_started) {
      printError("start()",  "Section was started already. Duplicated start is ignored.\n");
      return;
    }
	if (!m_is_set) {
      printError("start()",  "Section has not been defined correctly by setProperties(). start() is ignored.\n");
      m_is_healthy=false;
      return;
	}
    if (m_exclusive && ExclusiveStarted) {
      printError("start()",  "Section overlaps other exclusive section. start() is ignored.\n");
      m_is_healthy=false;
      return;
    }
    m_started = true;
    if (m_exclusive) ExclusiveStarted = true;
    m_startTime = getTime();

#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0)
		fprintf (stderr, "<PerfWatch::start> [%s] my_thread=%d, m_startTime=%f\n", m_label.c_str(), my_thread, m_startTime);
#endif

	if ( m_in_parallel ) {
		// The threads are active and running in parallel region
		startSectionParallel();
	} else {
		// The thread is running in serial region
		startSectionSerial();
	}

#ifdef USE_OTF
    if (m_is_OTF != 0) {
      int is_unit = statsSwitch();
      my_otf_event_start(my_rank, m_startTime, m_id, is_unit);
	}
#endif
  }


  void PerfWatch::startSectionSerial()
  {
	//	startSectionSerial() :	Only the master thread is active and is running in serial region
#ifdef DEBUG_PRINT_WATCH
    if (my_rank == 0) fprintf (stderr, "\t <startSectionSerial> [%s]\n", m_label.c_str());
#endif

    int is_unit = statsSwitch();
	if ( is_unit >= 2) {
#ifdef USE_PAPI
	#pragma omp parallel
	{
		//	parallel regionの全スレッドの処理
		int i_thread = omp_get_thread_num();
		struct pmlib_papi_chooser th_papi = my_papi;
		int i_ret;

		//	We call my_papi_bind_read() to preserve HWPC events for inclusive sections,
		//	in stead of calling my_papi_bind_start() which clears out the event counters.
		i_ret = my_papi_bind_read (th_papi.values, th_papi.num_events);
		if ( i_ret != PAPI_OK ) {
			fprintf(stderr, "*** error. <my_papi_bind_read> code: %d, thread:%d\n", i_ret, i_thread);
			//	PM_Exit(0);
		}

		#pragma ivdep
		for (int i=0; i<my_papi.num_events; i++) {
			my_papi.th_values[i_thread][i] = th_papi.values[i];
		}
	}	// end of #pragma omp parallel region
	#ifdef DEBUG_PRINT_PAPI_THREADS
		#pragma omp barrier
		if (my_rank == 0) {
			for (int j=0; j<num_threads; j++) {
				fprintf (stderr, "\t<startSectionSerial> [%s] my_papi.th_values[%d][*]:", m_label.c_str(), j);
				for (int i=0; i<my_papi.num_events; i++) {
					fprintf (stderr, "%llu, ", my_papi.th_values[j][i]);
				};	fprintf (stderr, "\n");
			}
		}
	#endif
#endif // USE_PAPI
	} else {
		;
	}
  }



  void PerfWatch::startSectionParallel()
  {
	//	startSectionParallel() : All threads are active and running in parallel region

	#ifdef DEBUG_PRINT_WATCH
	if (my_rank == 0) fprintf (stderr, "\t<startSectionParallel> [%s] my_thread=%d\n", m_label.c_str(), my_thread);
	#endif

    int is_unit = statsSwitch();
	if ( is_unit >= 2) {
#ifdef USE_PAPI
	struct pmlib_papi_chooser th_papi = my_papi;
	int i_ret;

	//	we call my_papi_bind_read() to preserve HWPC events for inclusive sections in stead of
	//	calling my_papi_bind_start() which clears out the event counters.
	i_ret = my_papi_bind_read (th_papi.values, th_papi.num_events);
	if ( i_ret != PAPI_OK ) {
		fprintf(stderr, "*** error. <my_papi_bind_read> code: %d, my_thread:%d\n", i_ret, my_thread);
		//	PM_Exit(0);
	}

	//	parallel regionの内側で呼ばれた場合は、my_threadはスレッドIDの値を持つ
	#pragma ivdep
	for (int i=0; i<my_papi.num_events; i++) {
		my_papi.th_values[my_thread][i] = th_papi.values[i];
	}
	#ifdef DEBUG_PRINT_PAPI_THREADS
	//	#pragma omp critical
	//	if (my_rank == 0) {
    //		fprintf (stderr, "\t\t my_thread=%d th_values[*]: ", my_thread);
	//		for (int i=0; i<my_papi.num_events; i++) {
	//			fprintf (stderr, "%llu, ", my_papi.th_values[my_thread][i] );
	//		};	fprintf (stderr, "\n");
	//	}
	#endif
#endif // USE_PAPI
	} else {
		;
	}

  }


  /// 測定区間ストップ.
  ///
  ///   @param[in] flopPerTask     測定区間の計算量(演算量Flopまたは通信量Byte)
  ///   @param[in] iterationCount  計算量の乗数（反復回数）
  ///
  ///   @note  引数はユーザ指定モードの場合にのみ利用され、計算量を
  ///          １区間１回あたりでflopPerTask*iterationCount として算出する。\n
  ///          HWPCによる自動算出モードでは引数は無視され、
  ///          内部で自動計測するHWPC統計情報から計算量を決定決定する。\n
  ///          レポート出力する情報の選択方法はPerfMonitor::stop()の規則による。\n
  ///
  void PerfWatch::stop(double flopPerTask, unsigned iterationCount)
  {
    if (!m_is_healthy) return;

    if (!m_started) {
      printError("stop()",  "Section has not been started\n");
      m_is_healthy=false;
      return;
    }

    m_stopTime = getTime();
    m_time += m_stopTime - m_startTime;
    m_count++;
    m_started = false;
    if (m_exclusive) ExclusiveStarted = false;

	if ( m_in_parallel ) {
		// The threads are active and running in parallel region
		stopSectionParallel(flopPerTask, iterationCount);
	} else {
		// The thread is running in serial region
		stopSectionSerial(flopPerTask, iterationCount);
	}
		// Move the following lines to the end, since sortPapiCounterList() overwrites them.
		//	my_papi.th_v_sorted[my_thread][0] = (double)m_count;
		//	my_papi.th_v_sorted[my_thread][1] = m_time;
		//	my_papi.th_v_sorted[my_thread][2] = m_flop;

	#ifdef DEBUG_PRINT_WATCH
	if (my_rank == 0) {
		fprintf (stderr, "<PerfWatch::stop> [%s] my_thread=%d, fPT=%e, itC=%u, m_count=%ld, m_time=%f, m_flop=%e\n",
			m_label.c_str(), my_thread, flopPerTask, iterationCount, m_count, m_time, m_flop);
		fprintf (stderr, "\t\t m_startTime=%f, m_stopTime=%f\n", m_startTime, m_stopTime);
	}
	#endif

#ifdef USE_OTF
    int is_unit = statsSwitch();
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
    		w = (flopPerTask * (double)iterationCount) / (m_stopTime-m_startTime);
		} else if ( (2 <= is_unit) && (is_unit <= Max_hwpc_output_group) ) {
			// 自動計測されたHWPCイベントを分析した計算speed
			sortPapiCounterList ();

			// is_unitが2,3の時、v_sorted[]配列の最後の要素は速度の次元を持つ
			// is_unitが4,5の時は...
			w = my_papi.v_sorted[my_papi.num_sorted-1] ;
		}
		my_otf_event_stop(my_rank, m_stopTime, m_id, is_unit, w);
	}
	#ifdef DEBUG_PRINT_OTF
    if (my_rank == 0) {
		fprintf (stderr, "\t <PerfWatch::stop> OTF [%s] w=%e, m_time=%f, m_flop=%e \n"
				, m_label.c_str(), w, m_time, m_flop );
    }
	#endif
#endif	// end of #ifdef USE_OTF

	// Remark: *.th_v_sorted[][] may have been overwritten by sortPapiCounterList() if m_is_OTF == 2.
	// So save these values here.
	my_papi.th_v_sorted[my_thread][0] = (double)m_count;
	my_papi.th_v_sorted[my_thread][1] = m_time;
	my_papi.th_v_sorted[my_thread][2] = m_flop;
  }


  void PerfWatch::stopSectionSerial(double flopPerTask, unsigned iterationCount)
  {
	//	Only the master thread is active and is running in serial region

    int is_unit = statsSwitch();
	if ( is_unit >= 2) {
#ifdef USE_PAPI
	if (my_papi.num_events > 0) {
	#pragma omp parallel 
	{
		int i_thread = omp_get_thread_num();
		struct pmlib_papi_chooser th_papi = my_papi;
		int i_ret;

		i_ret = my_papi_bind_read (th_papi.values, th_papi.num_events);
		if ( i_ret != PAPI_OK ) {
			printError("stop",  "<my_papi_bind_read> code: %d, i_thread:%d\n", i_ret, i_thread);
		}

		#pragma ivdep
		for (int i=0; i<my_papi.num_events; i++) {
			my_papi.th_accumu[i_thread][i] += (th_papi.values[i] - my_papi.th_values[i_thread][i]);
		}
	}	// end of #pragma omp parallel region

		#ifdef DEBUG_PRINT_PAPI_THREADS
		if (my_rank == 0) {
			/**
			for (int j=0; j<num_threads; j++) {
				fprintf (stderr, "\t<stopSectionSerial> [%s] my_papi.th_accumu[%d][*]:", m_label.c_str(), j);
				for (int i=0; i<my_papi.num_events; i++) {
					fprintf (stderr, "%llu, ", my_papi.th_accumu[j][i]);
				};	fprintf (stderr, "\n");
			}
			**/
			fprintf (stderr, "<stopSectionSerial> [%s] \n", m_label.c_str());
			for (int j=0; j<num_threads; j++) {
				fprintf (stderr, "\tmy_papi.th_values[%d][*]:", j);
				for (int i=0; i<my_papi.num_events; i++) {
					fprintf (stderr, "%llu, ", my_papi.th_values[j][i]);
				};	fprintf (stderr, "\n");
				fprintf (stderr, "\tmy_papi.th_accumu[%d][*]:", j);
				for (int i=0; i<my_papi.num_events; i++) {
					fprintf (stderr, "%llu, ", my_papi.th_accumu[j][i]);
				};	fprintf (stderr, "\n");
			}
		}
		#endif
	}	// end of if (my_papi.num_events > 0) block
#endif	// end of #ifdef USE_PAPI
	} else
	if ( (is_unit == 0) || (is_unit == 1) ) {
		// ユーザが引数で指定した計算量
		m_flop += flopPerTask * (double)iterationCount;
		#ifdef DEBUG_PRINT_WATCH
    	if (my_rank == 0) fprintf (stderr, "\t<stopSectionSerial> User mode m_flop=%e\n", m_flop);
		#endif
	}
  }


  void PerfWatch::stopSectionParallel(double flopPerTask, unsigned iterationCount)
  {
	//	All threads are active and running inside parallel region

    int is_unit = statsSwitch();
	if ( is_unit >= 2) {
#ifdef USE_PAPI
	if (my_papi.num_events > 0) {
	struct pmlib_papi_chooser th_papi = my_papi;
	int i_ret;

	i_ret = my_papi_bind_read (th_papi.values, th_papi.num_events);
	if ( i_ret != PAPI_OK ) {
		printError("stop",  "<my_papi_bind_read> code: %d, my_thread:%d\n", i_ret, my_thread);
	}

	#pragma ivdep
	for (int i=0; i<my_papi.num_events; i++) {
		my_papi.th_accumu[my_thread][i] += (th_papi.values[i] - my_papi.th_values[my_thread][i]);
	}

	#ifdef DEBUG_PRINT_PAPI_THREADS
	#pragma omp critical
	if (my_rank == 0) {
		fprintf (stderr, "\t<stopSectionParallel> [%s] my_thread=%d, my_papi.th_accumu[%d][*]:",
			m_label.c_str(), my_thread, my_thread);
			for (int i=0; i<my_papi.num_events; i++) {
				fprintf (stderr, "%llu, ", my_papi.th_accumu[my_thread][i]);
			};	fprintf (stderr, "\n");
	}
	#endif
	}	// end of if (my_papi.num_events > 0) {
#endif	// end of #ifdef USE_PAPI
	} else
	if ( (is_unit == 0) || (is_unit == 1) ) {
		// ユーザが引数で指定した計算量
		m_flop += flopPerTask * (double)iterationCount;
		#ifdef DEBUG_PRINT_WATCH
    	if (my_rank == 0) fprintf (stderr, "\t<stopSectionParallel> User mode: my_thread=%d, m_flop=%e\n", my_thread, m_flop);
		#endif
	}
  }


  /// 測定区間リセット
  ///
  void PerfWatch::reset()
  {
    //	m_started = true;
    //	m_startTime = getTime();

    m_time = 0.0;
    m_count = 0;
	m_flop = 0.0;

#ifdef USE_PAPI
	if (my_papi.num_events > 0) {
		for (int i=0; i<my_papi.num_events; i++) {
			my_papi.accumu[i] = 0.0;
			my_papi.v_sorted[i] = 0.0;
		}
	}
	#ifdef _OPENMP
			#pragma omp barrier
			#pragma omp master
			for (int j=0; j<num_threads; j++) {
			for (int i=0; i<my_papi.num_events; i++) {
				my_papi.th_accumu[j][i] = 0.0 ;
				my_papi.th_v_sorted[j][i] = 0.0 ;
			}
			}
	#endif
#endif

  }

} /* namespace pm_lib */
