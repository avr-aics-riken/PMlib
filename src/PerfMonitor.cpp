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

//	#ifdef _PM_WITHOUT_MPI_
//	#include "mpi_stubs.h"
//	#else
//	#include <mpi.h>
//	#endif

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
    std::string s;

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

	#ifdef USE_PAPI
	is_PAPI_enabled = true;
	#else
	is_PAPI_enabled = false;
	#endif

    #ifdef USE_OTF
    is_OTF_enabled = true;
    #else
    is_OTF_enabled = false;
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

	// 環境変数PMLIB_PRINT_VERSION が指定された場合、情報をstderrに出力する
    c_env = std::getenv("PMLIB_PRINT_VERSION");
    if (c_env != NULL) {
	if(my_rank == 0) {
		fprintf(stderr, "\tPMlib version %s is linked to the program.\n",
			 getVersionInfo().c_str());
	}
    }

    // Start m_watchArray[0] instance
    // m_watchArray[] は PerfWatch classである(PerfMonitorではない)ことに留意
    // PerfWatchのインスタンスは全部で m_nWatch 生成される
    // m_watchArray[0]  :PMlibが定義する特別な区間(Root)
    // m_watchArray[1 .. m_nWatch] :ユーザーが定義する各区間

    std::string label;
    label="Root Section";	// label="Total excution time";
#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
      fprintf(stderr, "<initialize> m_watchArray[0] instance: %s\n", label.c_str());
    }
#endif

    m_watchArray[0].initializeHWPC();
    int id = add_perf_label(label);	// The "Root Section" has id=0
    m_watchArray[id] = m_watchArray[0];
    m_nWatch++;
    m_watchArray[0].setProperties(label, id, CALC, num_process, my_rank, false);

    m_watchArray[0].initializeOTF();
    m_watchArray[0].start();

  }



  /// 測定区間にプロパティを設定.
  ///
  ///   @param[in] label ラベルとなる文字列
  ///   @param[in] type  測定量のタイプ(COMM:通信, CALC:計算)
  ///   @param[in] exclusive 排他測定フラグ。bool型(省略時true)、
  ///                        Fortran仕様は整数型(0:false, 1:true)
  ///
  void PerfMonitor::setProperties(const std::string& label, Type type, bool exclusive)
  {
    int id = add_perf_label(label);
    if (m_nWatch < 0) {
      fprintf(stderr, "\tPerfMonitor::setProperties() error. id=%u \n", id);
      PM_Exit(0);
    }

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
    m_watchArray[id].setProperties(label, id, type, num_process, my_rank, exclusive);
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



  /// 測定区間スタート
  ///
  ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
  ///
  ///
  void PerfMonitor::start (const std::string& label) {
    int id = find_perf_label(label);
    if (id < 0) {
      fprintf(stderr, "\t*** <start> Warning. undefined label [%s] is ignored by PMlib. \n", label.c_str());
      fprintf(stderr, "\t\t\tCheck the labels given to <setProperties>.\n");

    } else {
      m_watchArray[id].start();

      #ifdef DEBUG_PRINT_MONITOR
      int iret = MPI_Barrier(MPI_COMM_WORLD);
      if (my_rank == 0) {
        fprintf(stderr, "<start> [%s] id=%d\n", label.c_str(), id);
      }
      #endif
    }

  }
    

  /// 測定区間ストップ
  ///
  ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
  ///   @param[in] flopPerTask 測定区間の計算量(演算量Flopまたは通信量Byte):省略値0
  ///   @param[in] iterationCount  計算量の乗数（反復回数）:省略値1
  ///
  ///   @note  引数とレポート出力情報の関連はPerfWatch::stop()に詳しく説明されている。
  void PerfMonitor::stop(const std::string& label, double flopPerTask, unsigned iterationCount) {
    int id = find_perf_label(label);

    if (id < 0) {
      fprintf(stderr, "\t*** <stop> Warning. PMlib ignores undefined label [%s].\n", label.c_str());

    } else {
      m_watchArray[id].stop(flopPerTask, iterationCount);

      #ifdef DEBUG_PRINT_MONITOR
      int iret = MPI_Barrier(MPI_COMM_WORLD);
      if (my_rank == 0) {
        fprintf(stderr, "<stop> [%s] id=%d\n", label.c_str(), id);
      }
      #endif
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
    // gather() should be called only once, not more.

    if (m_gathered) {
        fprintf(stderr, "\tPerfMonitor::gather() error, already gathered\n");
        PM_Exit(0);
    }
    m_watchArray[0].stop(0.0, 1);

    if (m_nWatch == 0) return;


#ifdef DEBUG_PRINT_MONITOR
    int iret = MPI_Barrier(MPI_COMM_WORLD);
    if (my_rank == 0) {
      check_all_perf_label();
      for (int i=0; i<m_nWatch; i++) {
		std::string label;
        loop_perf_label(i, label);
		fprintf(stderr, "<gather> i=%d, %s \n", i, label.c_str());
      }
    }
#endif


    if (m_nWatch == 0) {
      //	No section has been defined via setProperties()
      return;
    }

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

    // 全プロセスの測定結果の集約を終了
    m_gathered = true;


    // 経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する
    double *m_tcost = NULL;
    if ( !(m_tcost = new double[m_nWatch]) ) PM_Exit(0);
    for (int i = 0; i < m_nWatch; i++) {
      PerfWatch& w = m_watchArray[i];
      //	if (!w.m_exclusive) continue;   // 
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
      //	if (!w.m_exclusive) continue;   // 
      if (w.m_label.empty()) continue;  //
      for (int j=i+1; j<m_nWatch; j++) {
          PerfWatch& q = m_watchArray[j];
          //	if (!q.m_exclusive) continue;   // 
          if (q.m_label.empty()) continue;  //
            if ( m_tcost[i] < m_tcost[j] ) {
              tmp_d=m_tcost[i]; m_tcost[i]=m_tcost[j]; m_tcost[j]=tmp_d;
              tmp_u=m_order[i]; m_order[i]=m_order[j]; m_order[j]=tmp_u;
            }
      }
    }

#ifdef USE_OTF
    if (is_OTF_enabled) {
      std::string label;
      for (int i=0; i<m_nWatch; i++) {
        loop_perf_label(i, label);
        m_watchArray[0].labelOTF (label, i);
      }
      m_watchArray[0].finalizeOTF();
    }
#endif
  }


  /// 測定結果の基本統計レポートを出力.
  ///
  ///   排他測定区間のみ
  ///   @param[in] fp       出力ファイルポインタ
  ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
  ///   @param[in] comments 任意のコメント
  ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  ///   @note 基本統計レポートは排他測定区間, 非排他測定区間をともに出力する。
  ///   MPIの場合、rank0プロセスの測定回数が１以上の区間のみを表示する。
  ///

  void PerfMonitor::print(FILE* fp, std::string hostname, const std::string comments, int seqSections)
  {
    if (my_rank != 0) return;
    if (m_nWatch == 0) {
      fprintf(fp, "\n# PMlib print():: No section has been defined via setProperties().\n");
      return;
    }
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
    
    // 測定時間の分母
    // initialize()からgather()までの区間（==Root区間）の測定時間を分母とする
    double tot = 0.0;
    if (0 == 0) {
      tot =  m_watchArray[0].m_time_av;
    } else {
      for (int i = 0; i < m_nWatch; i++) {
        if (m_watchArray[i].m_exclusive) {
          tot +=  m_watchArray[i].m_time_av;
        }
      }
    }

    int maxLabelLen = 0;
    for (int i = 0; i < m_nWatch; i++) {
      int labelLen = m_watchArray[i].m_label.size();
      if (!m_watchArray[i].m_exclusive) {
        labelLen = labelLen + 3;
        }
      maxLabelLen = (labelLen > maxLabelLen) ? labelLen : maxLabelLen;
    }
    maxLabelLen++;

	// PMlibインストール時のサポートプログラムモデルについての情報を出力する
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
    fprintf(fp, ", HWPC");
#else
    fprintf(fp, ", no-HWPC");
#endif
#ifdef USE_OTF
    fprintf(fp, ", OTF\n");
#else
    fprintf(fp, ", no-OTF\n");
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
#ifdef USE_PAPI
    m_watchArray[0].printHWPCHeader(fp);
#else
    fprintf(fp, "\n");
#endif

    fprintf(fp, "\n");
    fprintf(fp, "\tTotal execution time            = %12.6e [sec]\n", m_watchArray[0].m_time);
    fprintf(fp, "\tTotal time of measured sections = %12.6e [sec]\n", tot);
    fprintf(fp, "\n");
    fprintf(fp, "\tExclusive sections statistics per process and total job.\n");
    fprintf(fp, "\tInclusive sections are marked with (*)\n");
    fprintf(fp, "\n");

    // 演算数やデータ移動量の測定方法として、ユーザが明示的に指定する方法と、
    // HWPCによる自動測定が選択可能であるが、
    // どのような基準で演算数やデータ移動量を測定し結果を出力するかは
    // PerfWatch::stop() のコメントに詳しく書かれている

    is_unit = m_watchArray[0].statsSwitch();
	fprintf(fp, "\t%-*s|    call  |        accumulated time[sec]           ", maxLabelLen, "Section");
    if ( (is_unit == 0) || (is_unit == 1) ) {
      fprintf(fp, "| [user defined counter values ]\n");
    } else if ( (is_unit == 2) ) {
      fprintf(fp, "| [hardware counter flop counts]\n");
    } else if ( (is_unit == 3) ) {
      fprintf(fp, "| [hardware counter byte counts]\n");
    } else {
      fprintf(fp, "| [alternative hardware counter]\n");
    }
	fprintf(fp, "\t%-*s|          |      avr   avr[%%]      sdv   avr/call  ", maxLabelLen, "Label");
    fprintf(fp, "|      avr       sdv   speed\n");
	fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+----------------------------\n");
    
    // 表示
    double sum_time_comm = 0.0;
    double sum_time_flop = 0.0;
    double sum_time_other = 0.0;
    double sum_comm = 0.0;
    double sum_flop = 0.0;
    double sum_other = 0.0;
    std::string unit;
    double flops_w;
    std::string p_label;

    // 測定区間の時間と計算量を表示
    // 登録順で表示したい場合には seqSections=1 をprint()の引数で与える
    
    for (int j = 0; j < m_nWatch; j++) {
      if (j == 0) continue;
      int i;
      if (seqSections == 0) {
        i = m_order[j]; //	0:経過時間順
      } else {
        i = j; //	1:登録順で表示
      }

      PerfWatch& w = m_watchArray[i];
      if ( !(w.m_count > 0) ) continue;
      //	if ( !w.m_exclusive || w.m_label.empty()) continue;

      is_unit = w.statsSwitch();
      if (w.m_time_av == 0.0) {
        flops_w = 0.0;
      } else {
        flops_w = (w.m_count==0) ? 0.0 : w.m_flop_av/w.m_time_av;
      }

      //	double sum_time_av = 0.0;
        //	sum_time_av += w.m_time_av;

      if (w.m_exclusive) {
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
      }

      p_label = w.m_label;
      if (!w.m_exclusive) {
      p_label = w.m_label + "(*)" ;
      }

      // "\t%-*s: %8ld   %9.3e %6.2f  %8.2e  %9.3e    %8.3e  %8.2e %6.2f %s\n"
      fprintf(fp, "\t%-*s: %8ld   %9.3e %6.2f  %8.2e  %9.3e",
              maxLabelLen,
              //	w.m_label.c_str(), 
              p_label.c_str(), 
              w.m_count,            // 測定区間のコール回数
              w.m_time_av,          // 測定区間の時間(全プロセスの平均値)
              100*w.m_time_av/tot,  // 測定区間の時間/全区間（=Root区間）の時間
                                    // 分母 = initialize()からgather()までの区間
              w.m_time_sd,          // 標準偏差
              (w.m_count==0) ? 0.0 : w.m_time_av/w.m_count); // 1回あたりの時間


      if ( (0 <= is_unit) && (is_unit <= 4) ) {
        double uF = w.unitFlop(flops_w, unit, w.get_typeCalc(), is_unit);
        p_label = unit;		// 計算速度の単位
        if (!w.m_exclusive) {
          p_label = unit + "(*)";		// 非排他測定区間は単位表示が(*)
        }
        fprintf(fp, "    %8.3e  %8.2e %6.2f %s\n",
              w.m_flop_av,          // 測定区間の計算量(全プロセスの平均値)
              w.m_flop_sd,          // 計算量の標準偏差(全プロセスの平均値)
              uF,                   // 測定区間の計算速度(全プロセスの平均値)
              p_label.c_str());		// 計算速度の単位
      } else {
        fprintf(fp, "\n");
      }
    }
    
    fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+----------------------------\n");

    // Subtotal of the flop counts and/or byte counts
    if ( sum_time_flop > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Sections per process", "", sum_time_flop);
      double flop_serial = PerfWatch::unitFlop(sum_flop/sum_time_flop, unit, 1, 1);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive CALC sections-", sum_flop, flop_serial, unit.c_str());
    }
    if ( sum_time_comm > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Sections per process", "", sum_time_comm);
      double comm_serial = PerfWatch::unitFlop(sum_comm/sum_time_comm, unit, 0, 0);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive COMM sections-", sum_comm, comm_serial, unit.c_str());
    }
    if ( sum_time_other > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Sections per process", "", sum_time_other);
      double other_serial = PerfWatch::unitFlop(sum_other/sum_time_other, unit, 4, 4);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive sections only-", sum_other, other_serial, unit.c_str());
    }
    
    fputc('\t', fp); for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+----------------------------\n");

    // Job total flop counts and/or byte counts
    if ( sum_time_flop > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Sections total job", "", sum_time_flop);
      double sum_flop_job = (double)num_process*sum_flop;
      double flop_job = PerfWatch::unitFlop(sum_flop_job/sum_time_flop, unit, 1, 1);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive CALC sections-", sum_flop_job, flop_job, unit.c_str());
    }
    if ( sum_time_comm > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Sections total job", "", sum_time_comm);
      double sum_comm_job = (double)num_process*sum_comm;
      double comm_job = PerfWatch::unitFlop(sum_comm_job/sum_time_comm, unit, 0, 0);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive COMM sections-", sum_comm_job, comm_job, unit.c_str());
    }
    if ( sum_time_other > 0.0 ) {
      fprintf(fp, "\t%-*s %1s %9.3e", maxLabelLen+10, "Sections total job", "", sum_time_other);
      double sum_other_job = (double)num_process*sum_other;
      double other_job = PerfWatch::unitFlop(sum_other_job/sum_time_other, unit, 4, 4);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive sections only-", sum_other_job, other_job, unit.c_str());
    }

  }
  
  
  /// MPIランク別詳細レポート、HWPC詳細レポートを出力。 
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] legend int型 (省略可) HWPC記号説明の表示(0:なし、1:表示する)
  ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  ///   @note 本APIよりも先にPerfWatch::gather()を呼び出しておく必要が有る
  ///         HWPC値は各プロセス毎に子スレッドの値を合算して表示する
  ///   @note 詳細レポートは排他測定区間のみを出力する
  ///

  void PerfMonitor::printDetail(FILE* fp, int legend, int seqSections)
  {

    if (m_nWatch == 0) {
      if (my_rank == 0) {
      fprintf(fp, "\n# PMlib printDetail():: No section has been defined via setProperties().\n");
      }
      return;
    }
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
    
      // 測定時間の分母
      // initialize()からgather()までの区間（==Root区間）の測定時間を分母とする
      double tot = 0.0;
      if (0 == 0) {
        tot =  m_watchArray[0].m_time_av;
      } else {
        for (int i = 0; i < m_nWatch; i++) {
          if (m_watchArray[i].m_exclusive) {
            tot +=  m_watchArray[i].m_time_av;
          }
        }
      }

      for (int j = 0; j < m_nWatch; j++) {
        int i;
        if (seqSections == 0) {
          i = m_order[j]; //	0:経過時間順
        } else {
          i = j; //	1:登録順で表示
        }
		// report exclusive sections only
        if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printDetailRanks(fp, tot);
      }
    }

#ifdef USE_PAPI
    //	II. HWPC/PAPIレポート：HWPC計測結果を出力
    if (my_rank == 0) {
        fprintf(fp, "\n# PMlib hardware performance counter (HWPC) Report -------------------------\n");
    }

    for (int j = 0; j < m_nWatch; j++) {
        int i;
        if (seqSections == 0) {
          i = m_order[j]; //	0:経過時間順
        } else {
          i = j; //	1:登録順で表示
        }
		// report exclusive sections only
        if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printDetailHWPCsums(fp, m_watchArray[i].m_label);
    }

    if (my_rank == 0) {
      // HWPC Legend の表示はPerfMonitorクラスメンバとして分離する方が良いかも
      if (legend == 1) {
        m_watchArray[0].printHWPCLegend(fp);
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
  ///   @param[in] group  int型 (省略可) プロセスグループ番号
  ///   @param[in] legend int型 (省略可) HWPC記号説明の表示(0:なし、1:表示する)
  ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  ///   @note プロセスグループはp_group によって定義され、p_groupの値は
  ///   MPIライブラリが内部で定める大きな整数値を基準に決定されるため、
  ///   利用者にとって識別しずらい場合がある。
  ///   別に1,2,3,..等の昇順でプロセスグループ番号 groupをつけておくと
  ///   レポートが識別しやすくなる。
  ///
  void PerfMonitor::printGroup(FILE* fp, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group, int legend, int seqSections)
  {
    if (!m_gathered) {
      fprintf(stderr, "\n\t*** PerfMonitor gather() must be called before printGroup().\n");
      PM_Exit(0);
    }

    // check the size of the group
    int new_size, new_id;
    MPI_Group_size(p_group, &new_size);
    MPI_Group_rank(p_group, &new_id);

//  #define DEBUG_PRINT_MPI_GROUP 1
#ifdef DEBUG_PRINT_MPI_GROUP
    if (my_rank == 0) {
    	fprintf(fp, "<printGroup> MPI group:%d, ranks:%d \n", group, new_size);
    	for (int i = 0; i < new_size; i++) { fprintf(fp, "%3d ", pp_ranks[i]); }
    }
#endif

    // 	I. MPIランク別詳細レポート: MPIランク別測定結果を出力
    if (my_rank == 0) {
      fprintf(fp, "\n# PMlib Process Group [%5d] Elapsed time for individual MPI ranks --------\n\n", group);
    
      // 測定時間の分母
      // initialize()からgather()までの区間（==Root区間）の測定時間を分母とする
      double tot = 0.0;
      if (0 == 0) {
        tot =  m_watchArray[0].m_time_av;
      } else {
        for (int i = 0; i < m_nWatch; i++) {
          if (m_watchArray[i].m_exclusive) {
            tot +=  m_watchArray[i].m_time_av;
          }
        }
      }

      for (int j = 0; j < m_nWatch; j++) {
        int i;
        if (seqSections == 0) {
          i = m_order[j]; //	0:経過時間順
        } else {
          i = j; //	1:登録順で表示
        }
        if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printGroupRanks(fp, tot, p_group, pp_ranks);
      }
    }

#ifdef USE_PAPI
    //	II. HWPC/PAPIレポート：HWPC計測結果を出力
    if (my_rank == 0) {
      fprintf(fp, "\n# PMlib Process Group [%5d] hardware performance counter (HWPC) Report ---\n", group);
    }

    for (int j = 0; j < m_nWatch; j++) {
      int i;
      if (seqSections == 0) {
        i = m_order[j]; //	0:経過時間順
      } else {
        i = j; //	1:登録順で表示
      }
      if (!m_watchArray[i].m_exclusive) continue;
      m_watchArray[i].printGroupHWPCsums(fp, m_watchArray[i].m_label, p_group, pp_ranks);
    }

    if (my_rank == 0) {
      // HWPC Legend の表示はPerfMonitorクラスメンバとして分離する方が良いかも
      if (legend == 1) {
        m_watchArray[0].printHWPCLegend(fp);
        ;
      }
    }
#endif

  }


  /// MPI_Comm_split分離単位でのHWPC計測結果、MPIランク別詳細レポート出力
  /// for communicators created by MPI_Comm_split()
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] new_comm   MPI_Comm型 対応するcommunicator
  ///   @param[in] icolor int型 MPI_Comm_split()のカラー変数
  ///   @param[in] key    int型 MPI_Comm_split()のkey変数
  ///   @param[in] legend int型 (省略可) HWPC記号説明の表示(0:なし、1:表示する)
  ///   @param[in] seqSections (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  void PerfMonitor::printComm (FILE* fp, MPI_Comm new_comm, int icolor, int key, int legend, int seqSections)
  {

	int my_id, num_process;
	MPI_Group my_group;
	MPI_Comm_group(MPI_COMM_WORLD, &my_group);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &num_process);

	int ngroups;
	int *g_icolor;
	g_icolor = new int[num_process]();
	MPI_Gather(&icolor,1,MPI_INT, g_icolor,1,MPI_INT, 0, MPI_COMM_WORLD);

	if(my_id != 0) return;

	int *g_myid;
	int *p_gid;
	int *p_color;
	int *p_size;
	g_myid = new int[num_process]();
	p_gid  = new int[num_process]();
	p_color = new int[num_process]();
	p_size = new int[num_process]();

	std::list<int> c_list;
	std::list<int>::iterator it;
	for (int i=0; i<num_process; i++) {
		c_list.push_back(g_icolor[i]);
	}
	c_list.sort();
	c_list.unique();
	ngroups = c_list.size();

	int ip = 0;
	int gid = 0;
	for (it=c_list.begin(); it!=c_list.end(); it++ ) {
		int c = *it;
		p_gid[gid] = ip;
		p_color[gid] = c;

		for (int i=0; i<num_process; i++) {
			if( g_icolor[i] == c) { g_myid[ip++] = i; }
		}
		p_size[gid] = ip - p_gid[gid];
		gid++;
	}
//	#define DEBUG_PRINT_MPI_GROUP 1
#ifdef DEBUG_PRINT_MPI_GROUP
	fprintf(stderr, "<printComm> The number of produced MPI groups=%d\n", ngroups);
	for (int i=0; i<ngroups; i++) {
		fprintf(stderr, "group:%d, color=%d, size=%d, g_myid[%d]:",
			i, p_color[i], p_size[i], p_gid[i] );
		for (int j=0; j<p_size[i]; j++) {
			fprintf(stderr, " %d,", g_myid[p_gid[i]+j]);
		}
		fprintf(stderr, "\n");
	}
#endif

	MPI_Group new_group;
	for (int i=0; i<ngroups; i++) {
		int *p = &g_myid[p_gid[i]];
		MPI_Group_incl(my_group, p_size[i], p, &new_group);
		//	printGroup(stdout, new_group, new_comm, p, i);
		PerfMonitor::printGroup (stdout, new_group, new_comm, p, i, 0, seqSections);
	}

  }

} /* namespace pm_lib */

