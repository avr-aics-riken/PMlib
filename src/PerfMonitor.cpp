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

///@file   PerfMonitor.cpp
///@brief  PerfMonitor class

#include "PerfMonitor.h"
#include <time.h>


namespace pm_lib {
  
  /// 測定レベル制御変数.
  /// =0:測定なし
  /// =1:排他測定のみ
  /// =2:非排他測定も(ディフォルト)
  unsigned PerfMonitor::TimingLevel = 2;
  

  /// 全プロセスの全測定結果情報をマスタープロセス(0)に集約.
  ///
  ///   全計算時間用測定時計をストップ.
  ///
  void PerfMonitor::gather(void)
  {
    if (PerfMonitor::m_gathered) {
        fprintf(stderr, "\tPerfMonitor::gather() error, already gathered\n");
        PM_Exit(0);
    }
    m_total.stop(0.0, 1);
    for (int i = 0; i < m_nWatch; i++) {
        m_watchArray[i].gather();
    }
    m_gathered = true;

    // 経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する
    double *m_tcost = NULL;
    if ( !(m_tcost = new double[m_nWatch]) ) PM_Exit(0);
    for (int i = 0; i < m_nWatch; i++) {
      PerfWatch& w = m_watchArray[i];
      if (!w.m_exclusive) continue;   // 
      if ( w.m_count > 0 ) {
        m_tcost[i] = w.m_time_av;
      }
    }
    
    // 降順ソート O(n^2) Brute force　
    if ( !(m_order = new unsigned[m_nWatch]) ) PM_Exit(0);
    for (int i=0; i<m_nWatch; i++) {
      m_order[i] = i;
    }
    
    double tmp_d;
    unsigned tmp_u;
    for (int i=0; i<m_nWatch-1; i++) {
      PerfWatch& w = m_watchArray[i];
      if (!w.m_exclusive) continue;   // 
      if (w.m_label.empty()) continue;  //
      if (w.m_valid) {
        for (int j=i+1; j<m_nWatch; j++) {
          PerfWatch& q = m_watchArray[j];
          if (!q.m_exclusive) continue;   // 
          if (q.m_label.empty()) continue;  //
          if (q.m_valid) {
            if ( m_tcost[i] < m_tcost[j] ) {
              tmp_d=m_tcost[i]; m_tcost[i]=m_tcost[j]; m_tcost[j]=tmp_d;
              tmp_u=m_order[i]; m_order[i]=m_order[j]; m_order[j]=tmp_u;
            }
          }
        }
      }
    }

    // HWPCによるイベントカウンターの統計値を取得する
    for (int i=0; i<m_nWatch; i++) {
		if (!m_watchArray[i].m_exclusive) continue;
		m_watchArray[i].gatherHWPC();
    }

  }


  /// 測定結果を出力.
  ///
  ///   排他測定区間のみ
  ///   @param[in] fp           出力ファイルポインタ
  ///   @param[in] hostname     ホスト名
  ///   @param[in] operatorname 作業者名
  ///
  ///   @note ノード0以外は, 呼び出されてもなにもしない
  ///
  void PerfMonitor::print(FILE* fp, const std::string hostname, const std::string operatorname)
  {
    if (my_rank != 0) return;
    
    if (!m_gathered) {
      fprintf(stderr, "\tPerfMonitor::print() error, call gather() before print()\n");
      PM_Exit(0);
    }
    
    // タイムスタンプの取得
    struct tm *date;
    time_t now;
    int year, month, day;
    int hour, minute, second;
    
    time(&now);
    date = localtime(&now);
    
    year   = date->tm_year + 1900;
    month  = date->tm_mon + 1;
    day    = date->tm_mday;
    hour   = date->tm_hour;
    minute = date->tm_min;
    second = date->tm_sec;
    
    
    
    // 合計 > tot
    double tot = 0.0;
    int maxLabelLen = 0;
    for (int i = 0; i < m_nWatch; i++) {
      if (m_watchArray[i].m_exclusive) {
        tot +=  m_watchArray[i].m_time_av;
        int labelLen = m_watchArray[i].m_label.size();
        maxLabelLen = (labelLen > maxLabelLen) ? labelLen : maxLabelLen;
      }
    }
    maxLabelLen++;
    
    fprintf(fp, "\n#PMlib_report_section Basic Report type -------------\n");
    fprintf(fp, "\n\tReport of Timing Statistics PMlib version %s\n", PM_VERSION_NO);
    fprintf(fp, "\tOperator  : %s\n", operatorname.c_str());
    fprintf(fp, "\tHost name : %s\n", hostname.c_str());
    fprintf(fp, "\tDate      : %04d/%02d/%02d : %02d:%02d:%02d\n", year, month, day, hour, minute, second);
    fprintf(fp, "\n");
    
    fprintf(fp,"\tParallel Mode                    :   %s ", parallel_mode.c_str());
    if (parallel_mode == "Serial") {
      fprintf(fp, "\n");
    } else if (parallel_mode == "FlatMPI") {
      fprintf(fp, "(%d processes)\n", num_process);
    } else if (parallel_mode == "OpenMP") {
      fprintf(fp, "(%d threads)\n", num_threads);
    } else if (parallel_mode == "Hybrid") {
      fprintf(fp, "(%d processes x %d threads)\n", num_process, num_threads);
    } else {
      fprintf(fp, "\n\tError : invalid Parallel mode \n");
      PM_Exit(0);
    }
    fprintf(fp, "\n");
    
    fprintf(fp, "\tTotal execution time            = %12.6e [sec]\n", m_total.m_time);
    fprintf(fp, "\tTotal time of measured sections = %12.6e [sec]\n", tot);
    fprintf(fp, "\n");
    
    fprintf(fp, "\tStatistics per MPI process [Node Average]. Only the exclusive sections are listed.\n");
    
	fprintf(fp, "\t%-*s|    call  |        accumulated time[sec]           | [flop_counts or message_bytes ]\n",
		maxLabelLen, "Label");
	fprintf(fp, "\t%-*s|          |   avr     avr[%%]    sdv     avr/call   |   avr       sdv      speed\n",
		maxLabelLen, "");
	fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+----------------------------\n");
    
    
    
    // 表示
    double sum_time_av = 0.0;
    double sum_flop_av = 0.0;
    double sum_time_comm = 0.0;
    std::string unit;
    double flop_serial=0.0;
    
    // タイムコスト順表示
    // 登録順で表示したい場合には for (int i=0; i<m_nWatch; i++) { の様に直接 i でループすれば良い
    
    for (int j = 0; j < m_nWatch; j++) {
      int i = m_order[j];
      PerfWatch& w = m_watchArray[i];
      if (!w.m_exclusive) continue;   // 
      if (w.m_label.empty()) continue;  //
      if (w.m_valid) {
        if ( w.m_count > 0 ) {
          double flops_av;
          if (w.m_time_av == 0.0) {
            flops_av = 0.0;
          }
          else {
            flops_av = (w.m_count==0) ? 0.0 : w.m_flop_av/w.m_time_av;
          }
          fprintf(fp, "\t%-*s: %8ld   %9.3e %6.2f  %8.2e  %9.3e    %8.3e  %8.2e %6.2f %s\n",
                  maxLabelLen,
                  w.m_label.c_str(), 
                  w.m_count,            // 測定区間のコール回数
                  w.m_time_av,          // 測定区間の時間(ノード間平均値)
                  100*w.m_time_av/tot,  // 測定区間の時間/全排他測定区間の合計時間
                  w.m_time_sd,          // 標準偏差
                  (w.m_count==0) ? 0.0 : w.m_time_av/w.m_count, // 1回あたりの時間コスト
                  w.m_flop_av,          // ノード平均
                  w.m_flop_sd,          // 標準偏差
                  w.flops(flops_av, unit, w.get_typeCalc()), unit.c_str());
          sum_time_av += w.m_time_av;
          
          // 計算セクションのみ
          if ( w.m_typeCalc == 1 ) sum_flop_av += w.m_flop_av;
          
          // 非計算セクションのみ
          if ( w.m_typeCalc == 0 ) sum_time_comm += w.m_time_av; //w.m_time_comm;
        }
      }
      else {
        fprintf(fp, "\t%-*s: *** NA ***\n", maxLabelLen, w.m_label.c_str());
      }
    }
    
    fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+----------------------------\n");

    flop_serial = PerfWatch::flops(sum_flop_av/sum_time_av, unit, true);
	// remark. the values of sum_flop_av and flop_serial are not meaningful if both COMM and CALC are mixed
    fprintf(fp, "\t%-*s|%11s %9.3e                                %8.3e          %7.2f %s\n",
        maxLabelLen, "Process Total", "", sum_time_av, sum_flop_av, flop_serial, unit.c_str());
    
    // MPI 並列時には全プロセスの合計値も表示する
    int np;
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    if ( np > 1 ) {
      fprintf(fp,"\t%-*s  \t\t\t\t\t\t\t %7.2f %s\n", 
		maxLabelLen, "Aggregate performance of the job", flop_serial*(double)np, unit.c_str());
    }

  }
  
  
  
  /// HWPC計測結果、MPIランク別詳細レポートを出力. 非排他測定区間も出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///
  void PerfMonitor::printDetail(FILE* fp)
  {
    //	
	// 	詳細レポート　タイプA : MPIランク別測定結果を出力
	//  PerfWatch::gather() によって全プロセスの情報がrank 0に集計されている
    if (!m_gathered) {
      	fprintf(stderr, "\n\t*** PerfMonitor gather() must be called before printDetail().\n");
      	PM_Exit(0);
    }
    //	if (my_rank != 0) return;
	if (my_rank == 0) {
		fprintf(fp, "\n#PMlib_report_section Detail Report A ------------\n\n");
		fprintf(fp, "\tElapsed time variation over MPI ranks\n\n");
		double tot = 0.0;
		for (int i = 0; i < m_nWatch; i++) {
   	   		if (m_watchArray[i].m_exclusive) {
        		tot +=  m_watchArray[i].m_time_av;
      		}
    	}
		for (int i = 0; i < m_nWatch; i++) {
			if (!m_watchArray[i].m_exclusive) continue;
			m_watchArray[i].printDetailRanks(fp, tot);
		}
	}

    //	詳細レポート　タイプＢ：HWPC計測結果を出力
	//  プロセス情報の集計は下位ルーチンoutputPapiCounterList()で行われる
	if (my_rank == 0) {
		fprintf(fp, "\n#PMlib_report_section Detail Report B ------------\n\n");
		m_total.printHWPCHeader(fp);
	}

    for (int i = 0; i < m_nWatch; i++) {
		if (!m_watchArray[i].m_exclusive) continue;
		m_watchArray[i].printDetailHWPCsums(fp, m_watchArray[i].m_label);
    }

	if (my_rank == 0) {
		//	DEBUG. comment out to suppress printing the Legend.
		//	m_total.printHWPCLegend(fp);
	}
  }

} /* namespace pm_lib */


