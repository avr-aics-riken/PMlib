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

///@file   PerfMonitor.cpp
///@brief  PerfMonitor class

//
// 2015/2/2 Note for Debug. Delete the following comment lines later on.
// Shall we maintain two source files, one with MPI and another without MPI?
//
#ifdef _PM_WITHOUT_MPI_
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif

#include "PerfMonitor.h"
#include <time.h>
#include <unistd.h> // for gethostname() of FX10/K

namespace pm_lib {
  
  /// 測定レベル制御変数.
  /// =0:測定なし
  /// =1:排他測定のみ
  /// =2:非排他測定も(ディフォルト)
  unsigned PerfMonitor::TimingLevel = 2;


  /// 初期化.
  /// 測定区間数分の測定時計を準備.
  /// 最初にinit_nWatch区間分を確保し、不足したら動的にinit_nWatch追加する
  /// 全計算時間用測定時計をスタート.
  /// @param[in] （引数はオプション） init_nWatch 最初に確保する測定区間数
  ///
  /// @note 測定区間数 m_nWatch は不足すると動的に増えていく
  ///
  void PerfMonitor::initialize (int init_nWatch)
  {
    char* c_env;
    m_watchArray = new PerfWatch[init_nWatch];
    m_gathered = false;
    m_nWatch = 0 ;
    researved_nWatch = init_nWatch;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);

	#ifdef _OPENMP
    // if OMP_NUM_THREADS environment variable is not defined, set it to 1
    c_env = std::getenv("OMP_NUM_THREADS");
    if (c_env == NULL) {
        omp_set_num_threads(1);
    }
    num_threads = omp_get_max_threads();
	#else
    num_threads = 1;
	#endif

	// Preserve the parallel mode information while PMlib is being made.
	#ifdef _PM_WITHOUT_MPI_
	is_MPI_enabled = false;
	#else
	is_MPI_enabled = true;
	#endif
	#ifdef _OPENMP
	is_OpenMP_enabled = true;
	#else
	is_OpenMP_enabled = false;
	#endif
	#ifdef USE_PAPI
	is_PAPI_enabled = true;
	#else
	is_PAPI_enabled = false;
	#endif

    if( is_MPI_enabled == true) {
      if( num_threads == 1) {
        parallel_mode = "FlatMPI";
      } else {
        parallel_mode = "Hybrid";
      }
    }
    if( is_MPI_enabled == false) {
      if( num_threads == 1) {
        parallel_mode = "Serial";
      } else {
        parallel_mode = "OpenMP";
      }
    }

	// 環境変数PMLIB_PRINT_VERSION が指定された場合
	// PMlibがインストールされた時に指定された、サポートすべきプログラムモデル
	// についての情報を内部で保存しているが、その情報をstderrに出力する
    c_env = std::getenv("PMLIB_PRINT_VERSION");
    if (c_env != NULL) {
	if(my_rank == 0) {
		fprintf(stderr, "\tPMlib version %s built with the feature ",
			 getVersionInfo().c_str());
		fprintf(stderr, "MPI:%s, OpenMP:%s, HWPC/PAPI:%s\n"
			, is_MPI_enabled?"supported":"off"
			, is_OpenMP_enabled?"supported":"off"
			, is_PAPI_enabled?"supported":"off"
			);
	}
    }

    /// Start m_total
    /// m_total は PerfWatch classである(PerfMonitorではない)ことに留意
    m_total.initializeHWPC();
    m_total.setProperties("Total excution time", CALC, num_process, my_rank, false);
    m_total.start();
  }


  /// 並列モードを設定
  ///
  /// @param[in] p_mode 並列モード
  /// @param[in] n_thread
  /// @param[in] n_proc
  ///
  void PerfMonitor::setParallelMode(const std::string& p_mode, const int n_thread, const int n_proc)
  {
    parallel_mode = p_mode;
    if ((n_thread != num_threads) || (n_proc != num_process)) {
      if (my_rank == 0) {
        fprintf (stderr, "\t*** <setParallelMode> Warning. check n_thread:%d and n_proc:%d\n", n_thread, n_proc);
      }
    num_threads   = n_thread;
    num_process   = n_proc;
	}
  }


  /**
    * @brief PMlibバージョン情報の文字列を返す
   */
  std::string PerfMonitor::getVersionInfo(void)
  {
      std::string str(PM_VERSION_NO);
      return str;
  }


  /// 全プロセスの全測定結果情報をマスタープロセス(0)に集約.
  ///
  ///   全計算時間用測定時計をストップ.
  ///
  void PerfMonitor::gather(void)
  {
    if (m_gathered) {
        fprintf(stderr, "\tPerfMonitor::gather() error, already gathered\n");
        PM_Exit(0);
    }
    m_total.stop(0.0, 1);

    // 各測定区間のHWPCによるイベントカウンターの統計値を取得する
    for (int i=0; i<m_nWatch; i++) {
		m_watchArray[i].gatherHWPC();
    }

    // 各測定区間の測定結果情報をノード０に集約
    for (int i = 0; i < m_nWatch; i++) {
        m_watchArray[i].gather();
    }

    // 測定結果の平均値・標準偏差などの基礎的な統計計算
    for (int i = 0; i < m_nWatch; i++) {
      m_watchArray[i].statsAverage();
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
      //
      // we no longer skip the unbalanced calling counts, i.e. m_valid==false case
        for (int j=i+1; j<m_nWatch; j++) {
          PerfWatch& q = m_watchArray[j];
          if (!q.m_exclusive) continue;   // 
          if (q.m_label.empty()) continue;  //
            if ( m_tcost[i] < m_tcost[j] ) {
              tmp_d=m_tcost[i]; m_tcost[i]=m_tcost[j]; m_tcost[j]=tmp_d;
              tmp_u=m_order[i]; m_order[i]=m_order[j]; m_order[j]=tmp_u;
            }
        }
    }

  }


  /// 測定結果の基本統計レポートを出力.
  ///
  ///   排他測定区間のみ
  ///   @param[in] fp       出力ファイルポインタ
  ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
  ///   @param[in] comments 任意のコメント
  ///
  ///   @note ノード0以外は, 呼び出されてもなにもしない
  ///
  void PerfMonitor::print(FILE* fp, std::string hostname, const std::string comments)
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
    int is_unit;
    
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
    
    fprintf(fp, "\n# PMlib Basic Report -------------------------------------------------------\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tTiming Statistics Report from PMlib version %s\n", PM_VERSION_NO);
    fprintf(fp, "\tLinked PMlib supports: ");
#ifdef _PM_WITHOUT_MPI_
    fprintf(fp, "no-MPI");
#else
    fprintf(fp, "MPI");
#endif
#ifdef _OPENMP
    fprintf(fp, ", OpenMP");
#else
    fprintf(fp, ", no-OpenMP");
#endif
#ifdef USE_PAPI
    fprintf(fp, ", HWPC\n");
#else
    fprintf(fp, ", no-HWPC\n");
#endif


    if (hostname == "") {
      char hn[512];
      hn[0]='\0';
      if (gethostname(hn, sizeof(hn)) != 0) {
        fprintf(stderr, "<print> can not obtain hostname\n");
        hostname="unknown";
      } else {
        hostname=hn;
      }
    }
    fprintf(fp, "\tHost name : %s\n", hostname.c_str());
    fprintf(fp, "\tDate      : %04d/%02d/%02d : %02d:%02d:%02d\n", year, month, day, hour, minute, second);
    fprintf(fp, "\t%s\n", comments.c_str());

    fprintf(fp, "\tParallel Mode:   %s ", parallel_mode.c_str());
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
    fprintf(fp, "\tExclusive sections Statistics per process and per job.\n");
    fprintf(fp, "\n");

    // 演算数やデータ移動量の測定方法として、ユーザが明示的に指定する方法と、
    // HWPCによる自動測定が選択可能であるが、
    // どのような基準で演算数やデータ移動量を測定し結果を出力するかは
    // PerfWatch::stop() のコメントに詳しく書かれている

    is_unit = m_total.statsSwitch();
	fprintf(fp, "\t%-*s|    call  |        accumulated time[sec]           ", maxLabelLen, "Section");
    if ( (0 <= is_unit) && (is_unit <= 3) ) {
      fprintf(fp, "| [flop counts or byte counts ]\n");
    } else {
      fprintf(fp, "\n");
    }
	fprintf(fp, "\t%-*s|          |      avr   avr[%%]      sdv   avr/call  ", maxLabelLen, "Label");
    if ( (0 <= is_unit) && (is_unit <= 3) ) {
      fprintf(fp, "|      avr       sdv   speed\n");
    } else {
      fprintf(fp, "\n");
    }
	fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------");
    if ( (0 <= is_unit) && (is_unit <= 3) ) {
      fprintf(fp, "+----------------------------\n");
    } else {
      fprintf(fp, "\n");
    }
    
    // 表示
    double sum_time_av = 0.0;
    double sum_time_comm = 0.0;
    double sum_time_flop = 0.0;
    double sum_time_other = 0.0;
    double sum_comm = 0.0;
    double sum_flop = 0.0;
    double sum_other = 0.0;
    std::string unit;
    double flops_w;

    // 測定区間の時間と計算量を表示
    // タイムコスト順表示
    // 登録順で表示したい場合には for (int i=0; i<m_nWatch; i++) { の様に直接 i でループすれば良い
    
    for (int j = 0; j < m_nWatch; j++) {
      int i = m_order[j];
      PerfWatch& w = m_watchArray[i];
      if ( !w.m_exclusive || w.m_label.empty()) continue;
      if ( !(w.m_count > 0) ) continue;

      is_unit = w.statsSwitch();
      if (w.m_time_av == 0.0) {
        flops_w = 0.0;
      } else {
        flops_w = (w.m_count==0) ? 0.0 : w.m_flop_av/w.m_time_av;
      }

      sum_time_av += w.m_time_av;

      if ( (is_unit == 0) || (is_unit == 2) ) {
        sum_time_comm += w.m_time_av;
        sum_comm += w.m_flop_av;
      }
      if ( (is_unit == 1) || (is_unit == 3) ) {
        sum_time_flop += w.m_time_av;
        sum_flop += w.m_flop_av;
      }
      if (  is_unit == 4 ) {
        sum_time_other += w.m_time_av;
        sum_other += w.m_flop_av;
      }

      // "\t%-*s: %8ld   %9.3e %6.2f  %8.2e  %9.3e    %8.3e  %8.2e %6.2f %s\n"
      fprintf(fp, "\t%-*s: %8ld   %9.3e %6.2f  %8.2e  %9.3e",
              maxLabelLen,
              w.m_label.c_str(), 
              w.m_count,            // 測定区間のコール回数
              w.m_time_av,          // 測定区間の時間(全プロセスの平均値)
              100*w.m_time_av/tot,  // 測定区間の時間/全排他測定区間の合計時間
              w.m_time_sd,          // 標準偏差
              (w.m_count==0) ? 0.0 : w.m_time_av/w.m_count); // 1回あたりの時間

      if ( (0 <= is_unit) && (is_unit <= 3) ) {
        fprintf(fp, "    %8.3e  %8.2e %6.2f %s\n",
              w.m_flop_av,          // 測定区間の計算量(全プロセスの平均値)
              w.m_flop_sd,          // 計算量の標準偏差(全プロセスの平均値)
              w.unitFlop(flops_w, unit, w.get_typeCalc()), // 測定区間の計算速度(全プロセスの平均値)
              unit.c_str());		// 計算速度の単位
      } else {
        fprintf(fp, "\n");
      }
    }
    
    fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp, "+----------+----------------------------------------");
    if ( (0 <= is_unit) && (is_unit <= 3) ) {
	  fprintf(fp, "+----------------------------\n");
    } else {
	  fprintf(fp, "\n");
    }

    // Subtotal of the flop counts and/or byte counts
    if ( sum_time_flop > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Per Process flop sections", "", sum_time_flop);
      double flop_serial = PerfWatch::unitFlop(sum_flop/sum_time_flop, unit, 1);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "", sum_flop, flop_serial, unit.c_str());
    }
    if ( sum_time_comm > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Per Process byte sections", "", sum_time_comm);
      double comm_serial = PerfWatch::unitFlop(sum_comm/sum_time_comm, unit, 0);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "", sum_comm, comm_serial, unit.c_str());
    }
    
    fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp, "+----------+----------------------------------------");
    if ( (0 <= is_unit) && (is_unit <= 3) ) {
	  fprintf(fp, "+----------------------------\n");
    } else {
	  fprintf(fp, "\n");
    }

    // Job total flop counts and/or byte counts
    if ( sum_time_flop > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Job Total flop sections", "", sum_time_flop);
      double sum_flop_job = (double)num_process*sum_flop;
      double flop_job = PerfWatch::unitFlop(sum_flop_job/sum_time_flop, unit, 1);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "", sum_flop_job, flop_job, unit.c_str());
    }
    if ( sum_time_comm > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Job Total byte sections", "", sum_time_comm);
      double sum_comm_job = (double)num_process*sum_comm;
      double comm_job = PerfWatch::unitFlop(sum_comm_job/sum_time_comm, unit, 0);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "", sum_comm_job, comm_job, unit.c_str());
    }

  }
  
  
  /// MPIランク別詳細レポート、HWPC詳細レポートを出力。 非排他測定区間も出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] legend HWPC 記号説明の表示(0:なし、1:表示する) (optional)
  ///
  ///   @note 本APIよりも先にPerfWatch::gather()を呼び出しておく必要が有る
  ///         HWPC値は各プロセス毎に子スレッドの値を合算して表示する
  ///
  void PerfMonitor::printDetail(FILE* fp, int legend)
  {

    //   全プロセスの情報が PerfWatch::gather() によりrank 0に集計ずみのはず
    if (!m_gathered) {
      fprintf(stderr, "\n\t*** PerfMonitor gather() must be called before printDetail().\n");
      PM_Exit(0);
    }
    //        HWPC値を各スレッド毎にブレークダウンして表示する機能は今後開発
    //        lsum2p HWPCのスレッド毎表示(0:なし、1:表示する)

    // 	I. MPIランク別詳細レポート: MPIランク別測定結果を出力
    if (my_rank == 0) {
      if (is_MPI_enabled) {
        fprintf(fp, "\n# PMlib Process Report --- Elapsed time for individual MPI ranks ------\n\n");
      } else {
        fprintf(fp, "\n# PMlib Process Report ------------------------------------------------\n\n");
      }
      double tot = 0.0;
      for (int i = 0; i < m_nWatch; i++) {
        if (m_watchArray[i].m_exclusive) {
          tot +=  m_watchArray[i].m_time_av;
        }
      }
      for (int i = 0; i < m_nWatch; i++) {
        //	report both exclusive and non-exclusive sections
        //	if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printDetailRanks(fp, tot);
      }
    }

#ifdef USE_PAPI
    //	II. HWPC/PAPIレポート：HWPC計測結果を出力
    if (my_rank == 0) {
        fprintf(fp, "\n# PMlib hardware performance counter (HWPC) Report -------------------------\n");

        m_total.printHWPCHeader(fp);
    }

    for (int i = 0; i < m_nWatch; i++) {
        //	report both exclusive and non-exclusive sections
        //	if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printDetailHWPCsums(fp, m_watchArray[i].m_label);
    }

    if (my_rank == 0) {
      // HWPC Legend の表示はPerfMonitorクラスメンバとして分離する方が良いかも
      if (legend == 1) {
        m_total.printHWPCLegend(fp);
        ;
      }
    }
#endif

  }
  
  
  /// プロセスグループ単位でのHWPC計測結果、MPIランク別詳細レポート出力
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
  void PerfMonitor::printGroup(FILE* fp, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group, int legend)
  {
    if (!m_gathered) {
      fprintf(stderr, "\n\t*** PerfMonitor gather() must be called before printGroup().\n");
      PM_Exit(0);
    }

    // check the size of the group
    int new_size, new_id;
    MPI_Group_size(p_group, &new_size);
    MPI_Group_rank(p_group, &new_id);

    //	if (my_rank == 0) {
    //	fprintf(fp, "<printGroup> group %d has %d ranks\n", group, new_size);
    //	for (int i = 0; i < new_size; i++) { fprintf(fp, "%3d ", pp_ranks[i]); }
    //	}

    // 	I. MPIランク別詳細レポート: MPIランク別測定結果を出力
    if (my_rank == 0) {
      fprintf(fp, "\n# PMlib Process Group [%5d] Elapsed time for individual MPI ranks --------\n\n", group);
      double tot = 0.0;
      for (int i = 0; i < m_nWatch; i++) {
        if (m_watchArray[i].m_exclusive) {
          tot +=  m_watchArray[i].m_time_av;
        }
      }
      for (int i = 0; i < m_nWatch; i++) {
        if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printGroupRanks(fp, tot, p_group, pp_ranks);
      }
    }

#ifdef USE_PAPI
    //	II. HWPC/PAPIレポート：HWPC計測結果を出力
    if (my_rank == 0) {
      fprintf(fp, "\n# PMlib Process Group [%5d] hardware performance counter (HWPC) Report ---\n", group);
      m_total.printHWPCHeader(fp);
    }

    for (int i = 0; i < m_nWatch; i++) {
      if (!m_watchArray[i].m_exclusive) continue;
      m_watchArray[i].printGroupHWPCsums(fp, m_watchArray[i].m_label, p_group, pp_ranks);
    }

    if (my_rank == 0) {
      // HWPC Legend の表示はPerfMonitorクラスメンバとして分離する方が良いかも
      if (legend == 1) {
        m_total.printHWPCLegend(fp);
        ;
      }
    }
#endif

  }

} /* namespace pm_lib */

