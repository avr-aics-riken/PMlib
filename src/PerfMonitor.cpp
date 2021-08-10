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

///@file   PerfMonitor.cpp
///@brief  PerfMonitor class

//	either <mpi.h> or "mpi_stubs.h" is included in PerfWatch.cpp

#include "PerfMonitor.h"
#include <time.h>
#include <unistd.h> // for gethostname() of FX10/K
#include <cmath>
#include "power_obj_menu.h"

namespace pm_lib {

    /// shared map of section name and ID
    std::map<std::string, int > shared_map_sections;



  /// 初期化.
  /// 測定区間数分の測定時計を準備.
  /// 最初にinit_nWatch区間分を確保し、不足したら動的にinit_nWatch追加する
  /// 全計算時間用測定時計をスタート.
  /// @param[in] （引数はオプション） init_nWatch 最初に確保する測定区間数
  ///
  /// @note 測定区間数 m_nWatch は不足すると動的に増えていく
  ///
  void PerfMonitor::initialize (int inn)
  {
    char* cp_env;
	int iret;

	//	commenting the line below means "my_thread is defined as class varibale rather than local"
	//	int my_thread;	// Note this my_thread is just a local variable

	is_PMlib_enabled = true;
    cp_env = std::getenv("BYPASS_PMLIB");
    if (cp_env == NULL) {
		is_PMlib_enabled = true;
    } else {
		is_PMlib_enabled = false;
	}
    if (!is_PMlib_enabled) return;
    init_nWatch = inn;

    iret = MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	if (iret != 0) {
		fprintf(stderr, "*** PMlib error. <initialize> MPI_Comm_rank failed. iret=%d \n", iret);
		(void) MPI_Abort(MPI_COMM_WORLD, -999);
	}
    iret = MPI_Comm_size(MPI_COMM_WORLD, &num_process);
	if (iret != 0) {
		fprintf(stderr, "*** PMlib error. <initialize> MPI_Comm_size failed. iret=%d \n", iret);
		(void) MPI_Abort(MPI_COMM_WORLD, -999);
	}

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
    	// all threadprivate copies should debug print
		fprintf(stderr, "<PerfMonitor::initialize> starts.\n");
	}
	#endif

	#ifdef _OPENMP
    is_OpenMP_enabled = true;
	my_thread = omp_get_thread_num();
    num_threads = omp_get_max_threads();
	#else
    is_OpenMP_enabled = false;
	my_thread = 0;
    num_threads = 1;
	#endif

	// Preserve the parallel mode information while PMlib is being made.
	#ifdef DISABLE_MPI
	is_MPI_enabled = false;
	#else
	is_MPI_enabled = true;
	#endif

	#ifdef USE_PAPI
	is_PAPI_enabled = true;
	#else
	is_PAPI_enabled = false;
	#endif

	#ifdef USE_POWER
	is_POWER_enabled = true;
	#else
	is_POWER_enabled = false;
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

// Parse the Environment Variable HWPC_CHOOSER
	std::string s_chooser;
	std::string s_default = "USER";

    cp_env = NULL;
	cp_env = std::getenv("HWPC_CHOOSER");
	if (cp_env == NULL) {
		s_chooser = s_default;
	} else {
		s_chooser = cp_env;
		if (s_chooser == "FLOPS" ||
			s_chooser == "BANDWIDTH" ||
			s_chooser == "VECTOR" ||
			s_chooser == "CACHE" ||
			s_chooser == "CYCLE" ||
			s_chooser == "LOADSTORE" ||
			s_chooser == "USER" ) {
			;
		} else {
			printDiag("initialize()",  "unknown HWPC_CHOOSER value [%s]. the default value [%s] is set.\n", cp_env, s_default.c_str());
			s_chooser = s_default;
		}
	}
	env_str_hwpc = s_chooser;


// Start m_watchArray[0] instance
    // m_watchArray[] は PerfWatch classである(PerfMonitorではない)ことに留意
    // PerfWatchのインスタンスは全部で m_nWatch 生成される
    // m_watchArray[0]  :PMlibが定義する特別な背景区間(Root)
    // m_watchArray[1 .. m_nWatch] :ユーザーが定義する各区間

    std::string label;
    label="Root Section";

	// objects created by "new" operator can be accessed using pointer, not name.
    m_watchArray = new PerfWatch[init_nWatch];
    m_nWatch = 0 ;
    m_order = NULL;
	reserved_nWatch = init_nWatch;

    m_watchArray[0].my_rank = my_rank;
    m_watchArray[0].num_process = num_process;

// Note: HWPC, Power API,  and OTF are all initialized by a "Root Section" PerfWatch instance

// initialize HWPC interface structure
    m_watchArray[0].initializeHWPC();

// initialize Power API binding contexts
	int n = PerfMonitor::initializePOWER() ;

    m_watchArray[0].initializePowerWatch(n);


	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
		fprintf(stderr, "\t <initialize> map of my_rank=%d, my_thread=%d\n", my_rank, my_thread);
		#pragma omp critical
    	check_all_section_object();
		// end critical does not exist for C++
    }
	#endif

//	increment the section map object
    int id;
    id = add_section_object(label);	// id for "Root Section" should be 0

//	increment the shared sections as well
    int id_shared;
    id_shared = add_shared_section(label);

    m_nWatch++;
    m_watchArray[0].setProperties(label, id, CALC, num_process, my_rank, num_threads, false);

// initialize OTF manager
    m_watchArray[0].initializeOTF();

// start root section
    m_watchArray[0].start();
    is_Root_active = true;			// "Root Section" is now active

// start power measurement
	#ifdef USE_POWER
    m_watchArray[0].power_start( pm_pacntxt, pm_extcntxt, pm_obj_array, pm_obj_ext);
	#endif


// Parse the Environment Variable PMLIB_REPORT
	//	std::string s_chooser;
	s_default = "BASIC";

    cp_env = NULL;
	cp_env = std::getenv("PMLIB_REPORT");
	if (cp_env == NULL) {
		s_chooser = s_default;	// default setting
	} else {
		s_chooser = cp_env;
		if (s_chooser == "BASIC" ||
			s_chooser == "DETAIL" ||
			s_chooser == "FULL" ) {
			;
		} else {
			printDiag("initialize()",  "unknown PMLIB_REPORT value [%s]. the default value [%s] is set.\n", cp_env, s_default.c_str());
			s_chooser = s_default;
		}
	}
	env_str_report = s_chooser;
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

    if (!is_PMlib_enabled) return;

    if (label.empty()) {
      printDiag("setProperties()",  "label is blank. Ignoring this call.\n");
      return;
    }

	#ifdef _OPENMP
	my_thread = omp_get_thread_num();
	#else
	my_thread = 0;
	#endif


    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
		fprintf(stderr, "<setProperties> [%s] my_thread=%d \n",
			label.c_str(), my_thread);	//	bool is_PMlib_enabled?"true":"false"
	}
	#endif

    int id, id_shared;
    id = find_section_object(label);
	if (id < 0) {
    	id = add_section_object(label);
   		id_shared = add_shared_section(label);

    	#ifdef DEBUG_PRINT_MONITOR
		fprintf(stderr, "<setProperties> [%s] NEW section created by my_thread=%d as [%d] \n", label.c_str(), my_thread, id);
		#endif
	} else {
    	#ifdef DEBUG_PRINT_MONITOR
		fprintf(stderr, "<setProperties> [%s] section exists. my_thread=%d as [%d] \n", label.c_str(), my_thread, id);
		#endif
	}

//
// If short of memory, allocate new space
//	and move the existing PerfWatch class storage into the new address
//
    if ((m_nWatch+1) >= reserved_nWatch) {

      reserved_nWatch = m_nWatch + init_nWatch;
      PerfWatch* watch_more = new PerfWatch[reserved_nWatch];
      if (watch_more == NULL) {
        printDiag("setProperties()", "memory allocation failed. section [%s] is ignored.\n", label.c_str());
        reserved_nWatch = m_nWatch;
        return;
      }

      for (int i = 0; i < m_nWatch; i++) {
          watch_more[i] = m_watchArray[i];
      }

      delete [] m_watchArray;
      m_watchArray = NULL;
      m_watchArray = watch_more;
      watch_more = NULL;
      #ifdef DEBUG_PRINT_MONITOR
		if (my_rank == 0) {
		fprintf(stderr, "\t <setProperties> my_rank=%d, my_thread=%d expanded m_watchArray for [%s]. new reserved_nWatch=%d\n",
			my_rank, my_thread, label.c_str(), reserved_nWatch);
		}
      #endif
    }

    is_exclusive_construct = exclusive;
    m_nWatch++;
    m_watchArray[id].setProperties(label, id, type, num_process, my_rank, num_threads, exclusive);

    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
		fprintf(stderr, "Finishing <setProperties> [%s] id=%d my_rank=%d, my_thread=%d\n", label.c_str(), id, my_rank, my_thread);
    }
    #endif

  }



  /// 並列モードを設定
  ///
  /// @param[in] p_mode 並列モード
  /// @param[in] n_thread
  /// @param[in] n_proc
  ///
  void PerfMonitor::setParallelMode(const std::string& p_mode, const int n_thread, const int n_proc)
  {
    if (!is_PMlib_enabled) return;

    parallel_mode = p_mode;
    if ((n_thread != num_threads) || (n_proc != num_process)) {
      if (my_rank == 0) {
        fprintf (stderr, "\t*** <setParallelMode> Warning. check n_thread:%d and n_proc:%d\n", n_thread, n_proc);
      }
    num_threads   = n_thread;
    num_process   = n_proc;
	}
  }


  /// Get the power control mode and value
  ///
  ///   @param[in] knob   : power knob chooser
  ///   @param[out] value : current value for the knob
  ///
  void PerfMonitor::getPowerKnob(int knob, int & value)
  {
    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
      fprintf(stderr, "<PerfMonitor::getPowerKnob> is called. knob=%d \n",  knob);
    }
    #endif

	#ifdef USE_POWER
	int iret;
	if (level_POWER == 0) {
    	if (my_rank == 0) {
			fprintf(stderr, "*** Error PMlib. Set POWER_CHOOSER to activate <getPowerKnob> \n");
		}
	} else {
		iret = operatePowerKnob (knob, 0, value);
		if (iret != 0) fprintf(stderr, "warning <getPowerKnob> error code=%d\n", iret);
	}
	#else
	fprintf(stderr, "*** Warning PMlib. Power API is not included. <getPowerKnob> is ignored.\n");
    #endif
  }


  /// Set the power control mode and value
  ///
  ///   @param[in] knob  : power knob chooser
  ///   @param[in] value : new value for the knob
  ///
  void PerfMonitor::setPowerKnob(int knob, int value)
  {
    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
      fprintf(stderr, "<PerfMonitor::setPowerKnob> is called. knob=%d, value=%d \n",  knob, value);
    }
    #endif

	#ifdef USE_POWER
	int iret;
	if (level_POWER == 0) {
    	if (my_rank == 0) {
			fprintf(stderr, "*** Error PMlib. Set POWER_CHOOSER to activate <setPowerKnob> \n");
		}
	} else {
		iret = operatePowerKnob (knob, 1, value);
		if (iret != 0) fprintf(stderr, "warning <setPowerKnob> error code=%d\n", iret);
	}
	#else
	fprintf(stderr, "*** Warning PMlib. Power API is not included. <setPowerKnob> is ignored.\n");
    #endif
  }



  /// 測定区間スタート
  ///
  ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
  ///
  ///
  void PerfMonitor::start (const std::string& label)
  {
    if (!is_PMlib_enabled) return;

    int id;
    if (label.empty()) {
      printDiag("start()",  "label is blank. Ignored the call.\n");
      return;
    }
    id = find_section_object(label);

	#ifdef DEBUG_PRINT_MONITOR
	int i_thread;
	#ifdef _OPENMP
	i_thread = omp_get_thread_num();
	#else
	i_thread = 0;
	#endif
	if (my_rank == 0) {
		//	fprintf(stderr, "<start> [%s] section i_thread=%d : is %s \n",
		//		label.c_str(), i_thread, (id<0)?"NEW!":"already in");
		if (id < 0) {
			fprintf(stderr, "<start> [%s] i_thread=%d : NEW label is set.\n", label.c_str(), i_thread);
		} else {
			fprintf(stderr, "<start> [%s] i_thread=%d : label exists.\n", label.c_str(), i_thread);
		}
	}
	#endif

    if (id < 0) {
      // Create and set the property for this section
      PerfMonitor::setProperties(label);

      id = find_section_object(label);
      #ifdef DEBUG_PRINT_MONITOR
      if (my_rank == 0) {
        fprintf(stderr, "<start> created property for [%s] at class address %p\n",
				label.c_str(), &m_watchArray[id]);
      }
      #endif
    }
    is_exclusive_construct = true;

    m_watchArray[id].start();
	#ifdef USE_POWER
    m_watchArray[id].power_start( pm_pacntxt, pm_extcntxt, pm_obj_array, pm_obj_ext);
	#endif

    //	last_started_label = label;
    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
      fprintf(stderr, "<start> [%s] id=%d\n", label.c_str(), id);
    }
    #endif
  }


  /// 測定区間ストップ
  ///
  ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
  ///   @param[in] flopPerTask 測定区間の計算量(演算量Flopまたは通信量Byte):省略値0
  ///   @param[in] iterationCount  計算量の乗数（反復回数）:省略値1
  ///
  ///   @note  引数とレポート出力情報の関連は PerfMonitor.h に詳しく説明されている。
  ///
  void PerfMonitor::stop(const std::string& label, double flopPerTask, unsigned iterationCount)
  {
    if (!is_PMlib_enabled) return;

    int id;
    if (label.empty()) {
      printDiag("stop()",  "label is blank. Ignored the call.\n");
      return;
    }
    id = find_section_object(label);
    if (id < 0) {
      printDiag("stop()",  "label [%s] is undefined. This may lead to incorrect measurement.\n",
				label.c_str());
      return;
    }
    m_watchArray[id].stop(flopPerTask, iterationCount);
	#ifdef USE_POWER
    m_watchArray[id].power_stop( pm_pacntxt, pm_extcntxt, pm_obj_array, pm_obj_ext);
	#endif

    if (!is_exclusive_construct) {
      m_watchArray[id].m_exclusive = false;
    }
    is_exclusive_construct = false;

    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
      fprintf(stderr, "<stop> [%s] id=%d\n", label.c_str(), id);
    }
    #endif
  }


  /// 測定区間リセット
  ///
  ///   @param[in] label ラベル文字列。測定区間を識別するために用いる。
  ///
  ///
  void PerfMonitor::reset (const std::string& label)
  {
    if (!is_PMlib_enabled) return;

    int id;
    if (label.empty()) {
      printDiag("reset()",  "label is blank. Ignored the call.\n");
      return;
    }
    id = find_section_object(label);
    if (id < 0) {
      printDiag("reset()",  "label [%s] is undefined. This may lead to incorrect measurement.\n",
				label.c_str());
      return;
    }
    m_watchArray[id].reset();

    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
      fprintf(stderr, "<reset> [%s] id=%d\n", label.c_str(), id);
    }
    #endif
  }


  /// 全測定区間リセット
  /// ただしroot区間はresetされない
  ///
  ///
  void PerfMonitor::resetAll (void)
  {
    if (!is_PMlib_enabled) return;

    for (int i=0; i<m_nWatch; i++) {
      m_watchArray[i].reset();
    }

    #ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
      fprintf(stderr, "<resetAll> \n");
    }
    #endif
  }


  /**
    * @brief PMlibバージョン情報の文字列を返す
   */
  std::string PerfMonitor::getVersionInfo(void)
  {
      std::string str(PM_VERSION);
      return str;
  }


  ///	Stop the Root section, which means the ending of PMlib stats recording.
  ///
  ///
  void PerfMonitor::stopRoot (void)
  {
    if (!is_PMlib_enabled) return;

    if (is_Root_active) {
    	m_watchArray[0].stop(0.0, 1);

    	m_watchArray[0].power_stop( pm_pacntxt, pm_extcntxt, pm_obj_array, pm_obj_ext );
    	(void) finalizePOWER();

    	is_Root_active = false;
    }
  }

  /// Count the number of shared measured sections
  ///
  ///   @param[out] nSections     number of shared measured sections
  ///
  void PerfMonitor::countSections (int &nSections)
  {
	#ifdef DEBUG_PRINT_MONITOR
	check_all_shared_sections();
	#endif

	//	nSections = m_nWatch;	// the local value, i.e. local to each thread class object

	nSections = shared_map_sections.size();	// this is the shared value for all threads

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) { fprintf(stderr, "<countSections> nSections=%d \n" , nSections); }
	#endif
  }


  ///  Check if the section has been called inside of parallel region
  ///
  ///   @param[in] id         shared section number
  ///   @param[out] mid       class private section number
  ///   @param[out] inside     0/1 (0:serial region / 1:parallel region)
  ///
  void PerfMonitor::SerialParallelRegion (int &id, int &mid, int &inside)
  {
	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) { fprintf(stderr, "<SerialParallelRegion> section id=%d start \n" , id); }
	#endif

    if (!is_PMlib_enabled) return;

	int n_shared_sections = shared_map_sections.size();

    if ((id<0) || (n_shared_sections<=id)) {
		fprintf(stderr, "*** PMlib internal Error <SerialParallelRegion> section id=%d is out of range\n", id);
		inside=-1;
		return;
	}

	std::string s;

	mid=-1;
	for (auto it=shared_map_sections.begin(); it != shared_map_sections.end(); ++it)
	{
    	if (it->second == id) {
			s = it->first;
			mid = find_section_object(s);
			break;
		}
	}
	if ( (mid<0) || (mid>=n_shared_sections) ) {
		fprintf(stderr, "*** PMlib internal Error <SerialParallelRegion> section id=%d was not found\n", id);
		inside=-1;
		return;
	}

	if (m_watchArray[mid].m_in_parallel) {
		inside=1;
	} else {
		inside=0;
	}

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
		fprintf(stderr, "<SerialParallelRegion> id=%d [%s] is in %s region. mid=%d \n" , id, s.c_str(), m_watchArray[mid].m_in_parallel?"PARALLEL":"SERIAL", mid); }
	#endif
	return;
  }



  ///  Merge the parallel thread data into the master thread in three steps. 
  ///
  ///   @param[in] id    shared section number
  ///
  /// @note
  /// If the application calls PMlib from inside the parallel region,
  /// the PerfMonitor class must have been instantiated as threadprivate
  /// to preserve thread private my_papi.* memory storage.
  /// and mergeThreads() must be called from paralel region before calling report()
  ///
  void PerfMonitor::mergeThreads (int id)
  {
    if (!is_PMlib_enabled) return;
#ifdef _OPENMP
	int mid;
	bool in_parallel;
	in_parallel = omp_in_parallel();
	std::string s;

	for (auto it=shared_map_sections.begin(); it != shared_map_sections.end(); ++it)
	{
    	if (it->second == id) {
			s = it->first;
			mid = find_section_object(s);
			break;
		}
	}

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
		fprintf(stderr, "<mergeThreads> id=%d [%s] mid=%d in %s region. \n" , id, s.c_str(), mid, in_parallel?"PARALLEL":"SERIAL");
	}
	#endif


	#pragma omp barrier
	m_watchArray[mid].mergeMasterThread();
	#pragma omp barrier
	m_watchArray[mid].mergeParallelThread();
	#pragma omp barrier
	m_watchArray[mid].updateMergedThread();
	#pragma omp barrier

#endif
  }


  ///  Merge the parallel thread data of all sections
  ///  This function is obsolete and should not be necessary. Just leave here as a history
  ///
  void PerfMonitor::Obsolete_mergeThreads (void)
  {
    if (!is_PMlib_enabled) return;
#ifdef _OPENMP
	for (int i=0; i<m_nWatch; i++) {
		#pragma omp barrier
		m_watchArray[i].mergeMasterThread();
		#pragma omp barrier
		m_watchArray[i].mergeParallelThread();
		#pragma omp barrier
		m_watchArray[i].updateMergedThread();
	}
	#pragma omp barrier
#endif
  }



  /// 全プロセスの測定結果、全スレッドの測定結果を集約
  ///
  /// @note  以下の処理を行う。
  ///       各測定区間の全プロセスの測定結果情報をマスタープロセスに集約。
  ///       測定結果の平均値・標準偏差などの基礎的な統計計算。
  ///       経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
  ///       各測定区間のHWPCイベントの統計値を取得する。
  ///       各プロセスの全スレッド測定結果をマスタースレッドに集約
  ///
  void PerfMonitor::gather(void)
  {
    if (!is_PMlib_enabled) return;

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) { fprintf(stderr, "\n<PerfMonitor::gather> starts\n"); }
	#endif

    gather_and_stats();

    sort_m_order();

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) { fprintf(stderr, "<PerfMonitor::gather> finishes\n"); }
	#endif
  }


  /// 全プロセスの測定中経過情報を集約
  ///
  ///   @note  以下の処理を行う。
  ///    各測定区間の全プロセス途中経過状況を集約。
  ///    測定結果の平均値・標準偏差などの基礎的な統計計算。
  ///    各測定区間のHWPCイベントの統計値を取得する。
  ///   @note  gather_and_stats() は複数回呼び出し可能。
  ///
  void PerfMonitor::gather_and_stats(void)
  {

    if (!is_PMlib_enabled) return;

    if (m_nWatch == 0) return; // There is no section defined yet. This is basically an error case.

    // For each of the sections,
	// allgather the HWPC event values of all processes in MPI_COMM_WORLD.
	// Calibrate some numbers to represent the process value as the sum of thread values

    for (int i=0; i<m_nWatch; i++) {
      m_watchArray[i].gatherHWPC();
    }

	//   Allgather the process level basic statistics of m_time, m_flop, m_count
    for (int i = 0; i < m_nWatch; i++) {
        m_watchArray[i].gather();
    }

    //	summary stats including the average, standard deviation, etc.
    for (int i = 0; i < m_nWatch; i++) {
      m_watchArray[i].statsAverage();
    }

	//  summary stats of the estimated power consumption. Only the Root section does this.
    m_watchArray[0].gatherPOWER();

  }


  /// 経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
  /// Remark.
  /// 	Each process stores its own sorted list. Be careful when reporting from rank 0.
  ///
  ///
  void PerfMonitor::sort_m_order(void)
  {
    if (!is_PMlib_enabled) return;
    if (m_nWatch == 0) return;

    // 経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する
    // This delete/new may look redundant, but is needed if m_nWatch has
    // increased since last call...
    if ( m_order != NULL) { delete[] m_order; m_order = NULL; }
    if ( !(m_order = new unsigned[m_nWatch]) ) PM_Exit(0);

    for (int i=0; i<m_nWatch; i++) {
      m_order[i] = i;
    }

    double *m_tcost = NULL;
    if ( !(m_tcost = new double[m_nWatch]) ) PM_Exit(0);
    for (int i = 0; i < m_nWatch; i++) {
      PerfWatch& w = m_watchArray[i];
      if ( w.m_count_sum > 0 ) {
        m_tcost[i] = w.m_time_av;
      } else {
        m_tcost[i] = 0.0;
      }
    }
    // 降順ソート O(n^2) Brute force　
    double tmp_d;
    unsigned tmp_u;
    for (int i=0; i<m_nWatch-1; i++) {
      PerfWatch& w = m_watchArray[i];
      if (w.m_label.empty()) continue;  //
      for (int j=i+1; j<m_nWatch; j++) {
        PerfWatch& q = m_watchArray[j];
        if (q.m_label.empty()) continue;  //
        if ( m_tcost[i] < m_tcost[j] ) {
          tmp_d=m_tcost[i]; m_tcost[i]=m_tcost[j]; m_tcost[j]=tmp_d;
          tmp_u=m_order[i]; m_order[i]=m_order[j]; m_order[j]=tmp_u;
        }
      }
    }
    delete[] m_tcost; m_tcost = NULL;

	#ifdef DEBUG_PRINT_MONITOR
	(void) MPI_Barrier(MPI_COMM_WORLD);
	if (my_rank == 0) {
		fprintf(stderr, "<sort_m_order> my_rank=%d  m_order[*]:\n", my_rank );
		for (int j=0; j<m_nWatch; j++) {
		int k=m_order[j];
		fprintf(stderr, "\t\t m_order[%d]=%d time_av=%10.2e [%s]\n",
		j, k, m_watchArray[k].m_time_av, m_watchArray[k].m_label.c_str());
		}
	}
	#ifdef DEEP_DEBUG
	for (int i=0; i<num_process; i++) {
		(void) MPI_Barrier(MPI_COMM_WORLD);
    	if (i == my_rank) {
    		for (int j=0; j<m_nWatch; j++) {
			int k=m_order[j];
			fprintf(stderr, "\t\t rank:%d, m_order[%d]=%d time_av=%10.2e [%s]\n",
			my_rank, j, k, m_watchArray[k].m_time_av, m_watchArray[k].m_label.c_str());
			}
		}
	}
	if (my_rank == 0) fprintf(stderr, "<sort_m_order> ends");
	(void) MPI_Barrier(MPI_COMM_WORLD);
	#endif
	#endif

  }
  

  /// 測定結果の統計レポートを標準出力に表示する。
  ///   @note
  ///   基本統計レポート、MPIランク別詳細レポート、HWPC統計情報の詳細レポート、
  ///   スレッド別詳細レポートをまとめて出力する。
  ///   出力する内容は環境変数 PMLIB_REPORTの指定によってコントロールする。
  ///   PMLIB_REPORT=BASIC : 基本統計レポートを出力する。
  ///   PMLIB_REPORT=DETAIL: MPIランク別に経過時間、頻度、HWPC統計情報の詳細レポートを出力する。
  ///   PMLIB_REPORT=FULL： BASICとDETAILのレポートに加えて、
  ///		各MPIランクが生成した各並列スレッド毎にHWPC統計情報の詳細レポートを出力する。
  ///
  ///   @param[in] fp       出力ファイルポインタ
  ///
  void PerfMonitor::report(FILE* fp)
  {
	int iret;
    if (!is_PMlib_enabled) return;
	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank==0) {
    	fprintf(stderr, "<PerfMonitor::report> start \n");
	}
	#endif


	// BASIC report is always generated.
	PerfMonitor::print(fp, "", "", 0);

	// env_str_report should be one of {"BASIC" || "DETAIL" || "FULL"}
	if (env_str_report == "DETAIL" || env_str_report == "FULL") {
		PerfMonitor::printDetail(fp, 0, 0);
	}

	if (env_str_report == "FULL" ) {
    	for (int i = 0; i < num_process; i++) {
		PerfMonitor::printThreads(fp, i, 0);
		}
	}

	#ifdef DEBUG_PRINT_MONITOR
    fprintf(stderr, "<PerfMonitor::report> my_rank=%d reaching Barrier() \n", my_rank);
	iret = MPI_Barrier(MPI_COMM_WORLD);
	#endif

	// env_str_hwpc should be one of
	// {FLOPS| BANDWIDTH| VECTOR| CACHE| CYCLE| LOADSTORE| USER}
	if (env_str_hwpc != "USER" ) {
		PerfMonitor::printLegend(fp);
	}

  }




  /// 測定結果の基本統計レポートを出力.
  ///
  ///   @param[in] fp       出力ファイルポインタ
  ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
  ///   @param[in] comments 任意のコメント
  ///   @param[in] op_sort (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  ///   @note 基本統計レポートは排他測定区間, 非排他測定区間をともに出力する。
  ///   MPIの場合、rank0プロセスの測定回数が１以上の区間のみを表示する。
  ///
  void PerfMonitor::print(FILE* fp, std::string hostname, const std::string comments, int op_sort)
  {

	int my_thread;
	#ifdef _OPENMP
	#pragma omp barrier
	my_thread = omp_get_thread_num();		// a local variable
	#else
	my_thread = 0;		// a local variable
	#endif

    if (!is_PMlib_enabled) return;

    if (m_nWatch == 0) {
      if (my_rank == 0) {
        fprintf(fp, "\n\t#<PerfMonitor::print> No section has been defined.\n");
      }
      return;
    }
    gather();

    if (my_rank != 0) return;


    // 測定時間の分母
    // initialize()からgather()までの区間（==Root区間）の測定時間を分母とする
    double tot = m_watchArray[0].m_time_av;

    int maxLabelLen = 0;
    for (int i = 0; i < m_nWatch; i++) {
      int labelLen = m_watchArray[i].m_label.size();
      if (!m_watchArray[i].m_exclusive) {
        labelLen = labelLen + 3;
        }
      maxLabelLen = (labelLen > maxLabelLen) ? labelLen : maxLabelLen;
    }
    maxLabelLen++;

    /// ヘッダ部分を出力。
    PerfMonitor::printBasicHeader(fp, hostname, comments, tot);

    double sum_time_comm = 0.0;
    double sum_time_flop = 0.0;
    double sum_time_other = 0.0;
    double sum_comm = 0.0;
    double sum_flop = 0.0;
    double sum_other = 0.0;
    std::string unit;
    //	std::string p_label;

    // 各測定区間を出力
    // 計算量（演算数やデータ移動量）選択方法は PerfWatch::stop() のコメントに詳しく説明されている。
    PerfMonitor::printBasicSections (fp, maxLabelLen, tot, sum_flop, sum_comm, sum_other,
                                  sum_time_flop, sum_time_comm, sum_time_other, unit, op_sort);

    /// テイラ（合計）部分を出力。
    PerfMonitor::printBasicTailer (fp, maxLabelLen, sum_flop, sum_comm, sum_other,
                                  sum_time_flop, sum_time_comm, sum_time_other, unit);

    PerfMonitor::printBasicHWPC (fp, maxLabelLen, op_sort);

    PerfMonitor::printBasicPower (fp, maxLabelLen, op_sort);

  }



/// Report the BASIC HWPC statistics of the master process
///
///   @param[in] fp       	report file pointer
///   @param[in] maxLabelLen    maximum label string field length
///   @param[in] op_sort 	sorting option (0:sorted by seconds, 1:listed order)
///
void PerfMonitor::printBasicHWPC (FILE* fp, int maxLabelLen, int op_sort)
{
#ifdef USE_PAPI
	if (m_watchArray[0].my_papi.num_events == 0) return;
	if (env_str_hwpc == "USER" ) return;

	m_watchArray[0].printBasicHWPCHeader(fp, maxLabelLen);

    for (int j = 0; j < m_nWatch; j++) {
        int i;
        if (op_sort == 0) {
          i = m_order[j];	// sorted by elapsed time
        } else {
          i = j;			// listed order
        }
        if (j==0)  continue;
		// report exclusive sections only
        //	if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printBasicHWPCsums(fp, maxLabelLen);
    }

	for (int i=0; i< maxLabelLen; i++) { fputc('-', fp); }  fputc('+', fp);
	for (int i=0; i<(m_watchArray[0].my_papi.num_sorted*11); i++) { fputc('-', fp); } fprintf(fp, "\n");
#endif
}


/// Report the BASIC power consumption statistics of the master node
///
///   @param[in] fp       	report file pointer
///   @param[in] maxLabelLen    maximum label string field length
///   @param[in] op_sort 	sorting option (0:sorted by seconds, 1:listed order)
///
///	  @note   remark that power consumption is measured per node, not per process
///
void PerfMonitor::printBasicPower(FILE* fp, int maxLabelLen, int op_sort)
{
    if (!is_PMlib_enabled) return;
#ifdef USE_POWER
//
// Fugaku local implementation
//
	int n_parts;
    std::string p_label;


	if (level_POWER == 0) return;

	//original power object array i.e. std::string p_obj_name[Max_power_stats] =
	// power.num_power_stats == Max_power_stats == 20

	static double sorted_joule[Max_power_stats];
	static std::string p_obj_shortname[Max_power_stats] =
	{
		"  total ",	// 0
		"CMG0",	// 1
		"CMG1",	// 2
		"CMG2",	// 3
		"CMG3",	// 4
		"L2CMG0",	// 5
		"L2CMG1",	// 6
		"L2CMG2",	// 7
		"L2CMG3",	// 8
		"Acore0",	// 9
		"Acore1",	// 10
		"TofuD ",	// 11
		"UnCMG",	// 12
		"MEM0 ",	// 13
		"MEM1 ",	// 14
		"MEM2 ",	// 15
		"MEM3 ",	// 16
		"PCI  ",	// 17
		"TofuOpt ",	// 18
		"P.meter"	// 19 : reserved for the measured device. we decided not to report this value.
	};
	static std::string sorted_obj_name[Max_power_stats];

	n_parts = 0;

	if (level_POWER == 1) {	// total, CMG+L2, MEMORY, TF+A+U, P.meter
		p_label = "NODE";
		n_parts = 4;
		sorted_obj_name[0] = "  total ";
		sorted_obj_name[1] = "| CMG+L2";
		sorted_obj_name[2] = "  MEMORY";
		sorted_obj_name[3] = " TF+A+U ";
		//	sorted_obj_name[4] = " P.meter";
	} else
	if (level_POWER == 2) {	// total, CMG0+L2, CMG1+L2, CMG2+L2, CMG3+L2, MEM0, MEM1, MEM2, MEM3, TF+A+U, P.meter
		p_label = "NUMA";
		n_parts = 10;
		sorted_obj_name[0] = "  total ";
		sorted_obj_name[1] = "|" + p_obj_shortname[1] + "+L2";	// "|CMG0+L2"
		for (int i=2; i<5; i++) {
			sorted_obj_name[i] = p_obj_shortname[i] + "+L2";	// "CMG1+L2", "CMG2+L2", "CMG3+L2"
		}
		for (int i=5; i<9; i++) {
			sorted_obj_name[i] = p_obj_shortname[i+8];
		}
		sorted_obj_name[9] = " TF+A+U ";					// TF+A+U
		//	sorted_obj_name[10] = p_obj_shortname[19];

	} else
	if (level_POWER == 3) {
		p_label = "PARTS";
		n_parts = 19;

		for (int i=0; i<n_parts; i++) {
		sorted_obj_name[i] = p_obj_shortname[i];
		}
		sorted_obj_name[1] = "|   " + p_obj_shortname[1];	// trick to add "|"
	}

	fprintf(fp, "\n");
	fprintf(fp, "# PMlib Power Consumption report per node basis ---------------------------------- #\n");
	fprintf(fp, "\n");
    fprintf(fp, "\tReport of the master node is generated for POWER_CHOOSER=%s option.\n\n", p_label.c_str());


    for (int i=0; i< maxLabelLen; i++) { fputc(' ', fp); }
	fprintf(fp, "   Estimated power inside node [W]\n");

	fprintf(fp, "Section"); for (int i=7; i< maxLabelLen; i++) { fputc(' ', fp); }		fputc('|', fp);
	for (int i=0; i<n_parts-1; i++) { fprintf(fp, "%8s", sorted_obj_name[i].c_str()); }
	fprintf(fp, " %8s| Energy[Wh] \n", sorted_obj_name[n_parts-1].c_str());

    for (int i=0; i< maxLabelLen; i++) { fputc('-', fp); }	; fprintf(fp, "+--------+");
    for (int i=0; i<(n_parts-1)*8; i++) { fputc('-', fp); }	 ; fprintf(fp, "+----------\n");

	// actual records
    for (int j=0; j<m_nWatch; j++)
	{
		int m;
		if (op_sort == 0) {
			m = m_order[j];
		} else {
			m = j;
		}
		if (m == 0) continue;
		PerfWatch& w = m_watchArray[m];

		if (level_POWER == 1) {	// total, CMG+L2, MEMORY, TF+A+U
			sorted_joule[0] = w.my_power.w_accumu[0];
			sorted_joule[1] = 0.0;
			for (int i=1; i<9; i++) {
				sorted_joule[1] += w.my_power.w_accumu[i];
			}
			sorted_joule[2] = 0.0;
			for (int i=13; i<17; i++) {
				sorted_joule[2] += w.my_power.w_accumu[i];
			}
			sorted_joule[3] = w.my_power.w_accumu[9] + w.my_power.w_accumu[10] + w.my_power.w_accumu[11]
					+ w.my_power.w_accumu[12] + w.my_power.w_accumu[17] + w.my_power.w_accumu[18] ;
	
		} else
		if (level_POWER == 2) {	// total, CMG0+L2, CMG1+L2, CMG2+L2, CMG3+L2, MEM0, MEM1, MEM2, MEM3, TF+A+U
			// total value
			sorted_joule[0] = w.my_power.w_accumu[0];
			// CMGn + L2$ value
			for (int i=1; i<5; i++) {
				sorted_joule[i] = w.my_power.w_accumu[i] + w.my_power.w_accumu[i+4];
			}
			// memory values
			for (int i=5; i<9; i++) {
				sorted_joule[i] = w.my_power.w_accumu[i+8];
			}
			// "Tofu+AC" == Acore0 +  Acore1 + Tofu + Uncmg + PCI + TofuOpt
			sorted_joule[9] = w.my_power.w_accumu[9] + w.my_power.w_accumu[10] + w.my_power.w_accumu[11]
					+ w.my_power.w_accumu[12] + w.my_power.w_accumu[17] + w.my_power.w_accumu[18] ;
			//Physically measured value by the power meter
			//	sorted_joule[10] = w.my_power.w_accumu[19];
	
		} else
		if (level_POWER == 3) {
			for (int i=0; i<n_parts; i++) {
				sorted_joule[i] = w.my_power.w_accumu[i];
			}
		}

		p_label = w.m_label;
		if (!w.m_exclusive) {
			p_label = w.m_label + "(*)";
		}
		fprintf(fp, "%-*s:", maxLabelLen, p_label.c_str() );
		for (int i=0; i<n_parts; i++) {
			fprintf(fp, "%7.1f ",  sorted_joule[i]/w.m_time_av);	// Watt value
		}
		fprintf(fp, "  %8.2e",  sorted_joule[0]/3600.0);			// Watt-Hour energy value
		fprintf(fp, "\n");
	}

    for (int i=0; i< maxLabelLen; i++) { fputc('-', fp); }	; fprintf(fp, "+--------+");
    for (int i=0; i<(n_parts-1)*8; i++) { fputc('-', fp); }	 ; fprintf(fp, "+----------\n");

	double t_joule;
	int nnodes;
	int np_per_node;
	int iret;

    char* cp_env;
    cp_env = NULL;
	cp_env = std::getenv("PJM_PROC_BY_NODE");	// Fugaku job manager holds this variable
	if (cp_env == NULL) {
		np_per_node = 1;
	} else {
		np_per_node = atoi(cp_env);;
	}
    #ifdef DEBUG_PRINT_MONITOR
	if (my_rank == 0) {
		fprintf(fp, "<PerfMonitor::printBasicPower> translated PJM_PROC_BY_NODE=%d \n", np_per_node);
	}
	#endif
	//	nnodes=num_process/np_per_node;
	nnodes=(num_process-1)/np_per_node+1;

	fprintf(fp, "\n");
	fprintf(fp, "\t The aggregate power consumption of %d processes on %d nodes =", num_process, nnodes);
	// m_power_av is the average value of my_power.w_accumu[Max_power_stats-1]; for all processes
	t_joule = (m_watchArray[0].m_power_av * nnodes);
	fprintf(fp, "%10.2e [J] == %10.2e [Wh]\n", t_joule, t_joule/3600.0);

#endif
}


  /// MPIランク別詳細レポート、HWPC詳細レポートを出力。
  ///
  ///   @param[in] fp           出力ファイルポインタ
  ///   @param[in] legend int型 (省略可) HWPC記号説明の表示(0:なし、1:表示する)
  ///   @param[in] op_sort      (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  ///   @note 詳細レポートは排他測定区間のみを出力する
  ///         HWPC値は各プロセス毎に子スレッドの値を合算して表示する
  ///
  void PerfMonitor::printDetail(FILE* fp, int legend, int op_sort)
  {

    if (!is_PMlib_enabled) return;

    if (m_nWatch == 0) {
      if (my_rank == 0) {
      fprintf(fp, "\n# PMlib printDetail():: No section has been defined via setProperties().\n");
      }
      return;
    }

    gather();

    if (my_rank != 0) return;
	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) fprintf(stderr, "<printDetail> \n");
	#endif

    // 	I. MPIランク別詳細レポート: MPIランク別測定結果を出力
      if (is_MPI_enabled) {
        fprintf(fp, "\n## PMlib Process Report --- Elapsed time for individual MPI ranks ------\n\n");
      } else {
        fprintf(fp, "\n## PMlib Process Report ------------------------------------------------\n\n");
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

    // 測定区間の時間と計算量を表示。表示順は引数 op_sort で指定されている。
      for (int j = 0; j < m_nWatch; j++) {
        int i;
        if (op_sort == 0) {
          i = m_order[j]; //	0:経過時間順
        } else {
          i = j; //	1:登録順で表示
        }
        if (i == 0) continue;
		// Policy change
		// Both of exclusive sections and inclusive sections will be reported.
        //	if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printDetailRanks(fp, tot);
      }


#ifdef USE_PAPI
    //	II. HWPC/PAPIレポート：HWPC計測結果を出力
	if (m_watchArray[0].my_papi.num_events == 0) return;
	if (env_str_hwpc == "USER" ) return;
    fprintf(fp, "\n## PMlib hardware performance counter (HWPC) report for individual MPI ranks ---------\n\n");
    fprintf(fp, "\tThe HWPC stats report for HWPC_CHOOSER=%s is generated.\n\n", env_str_hwpc.c_str());


    for (int j = 0; j < m_nWatch; j++) {
        int i;
        if (op_sort == 0) {
          i = m_order[j]; //	0:経過時間順
        } else {
          i = j; //	1:登録順で表示
        }
		// Policy change
		// Both of exclusive sections and inclusive sections will be reported.
        //	if (!m_watchArray[i].m_exclusive) continue;
        m_watchArray[i].printDetailHWPCsums(fp, m_watchArray[i].m_label);
    }

#endif
  }


  /// 指定プロセス内のスレッド別詳細レポートを出力。
  ///
  ///   @param[in] fp           出力ファイルポインタ
  ///   @param[in] rank_ID      出力対象プロセスのランク番号
  ///   @param[in] op_sort      測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  void PerfMonitor::printThreads(FILE* fp, int rank_ID, int op_sort)
  {

    if (!is_PMlib_enabled) return;

    if (m_nWatch == 0) {
      if (my_rank == 0) printDiag("PerfMonitor::printThreads",  "No section is defined. No report.\n");
      return;
    }

    //	gather();	// m_order[] should not be overwriten/re-created for each thread
    gather_and_stats();

    if (my_rank == 0) {
      if (is_MPI_enabled) {
        fprintf(fp, "\n## PMlib Thread Report for MPI rank %d  ----------------------\n\n", rank_ID);
      } else {
        fprintf(fp, "\n## PMlib Thread Report for the single process run ---------------------\n\n");
      }
    }

    // 測定区間の時間と計算量を表示。表示順は引数 op_sort で指定されている。
    for (int j = 0; j < m_nWatch; j++) {
      int i;
      if (op_sort == 0) {
          i = m_order[j]; //	0:経過時間順
      } else {
          i = j; //	1:登録順で表示
      }
      if (i == 0) continue;	// 区間0 : Root区間は出力しない
		// Policy change
		// Both of exclusive sections and inclusive sections will be reported.
        //	if (!m_watchArray[i].m_exclusive) continue;
      if (!(m_watchArray[i].m_count_sum > 0)) continue;

      m_watchArray[i].printDetailThreads(fp, rank_ID);
    }
	#ifdef DEBUG_PRINT_MONITOR
	fprintf(stderr, "<printThreads> my_rank=%d finishing rank_ID=%d\n", my_rank, rank_ID);
	#endif

  }



  /// HWPC 記号の説明表示を出力。
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///
  void PerfMonitor::printLegend(FILE* fp)
  {
    if (!is_PMlib_enabled) return;
    if (!is_PAPI_enabled) return;

    if (my_rank == 0) {
      fprintf(fp, "\n# PMlib Legend - the symbols used in the reports  ----------------------\n");
      m_watchArray[0].printHWPCLegend(fp);
    }
  }



  /// 指定するMPIプロセスグループ毎にMPIランク詳細レポートを出力。
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] p_group  MPI_Group型 groupのgroup handle
  ///   @param[in] p_comm   MPI_Comm型 groupに対応するcommunicator
  ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
  ///   @param[in] group  int型 (省略可) プロセスグループ番号
  ///   @param[in] legend int型 (省略可) HWPC記号説明の表示(0:なし、1:表示する)
  ///   @param[in] op_sort (省略可)測定区間の表示順 (0:経過時間順、1:登録順で表示)
  ///
  ///   @note プロセスグループはp_group によって定義され、p_groupの値は
  ///   MPIライブラリが内部で定める大きな整数値を基準に決定されるため、
  ///   利用者にとって識別しずらい場合がある。
  ///   別に1,2,3,..等の昇順でプロセスグループ番号 groupをつけておくと
  ///   レポートが識別しやすくなる。
  ///
  void PerfMonitor::printGroup(FILE* fp, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group, int legend, int op_sort)
  {

    if (!is_PMlib_enabled) return;

    //	gather(); is always called by print()

    // check the size of the group
    int new_size, new_id;
    MPI_Group_size(p_group, &new_size);
    MPI_Group_rank(p_group, &new_id);

	#ifdef DEBUG_PRINT_MPI_GROUP
    //	if (my_rank == 0) {
    	fprintf(fp, "<printGroup> MPI group:%d, new_size=%d, ranks: ", group, new_size);
    	for (int i = 0; i < new_size; i++) { fprintf(fp, "%3d ", pp_ranks[i]); }
    //	}
	#endif

    // 	I. MPIランク別詳細レポート: MPIランク別測定結果を出力
    if (my_rank == 0) {
      fprintf(fp, "\n## PMlib Process Group [%5d] Elapsed time for individual MPI ranks --------\n\n", group);

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
        if (op_sort == 0) {
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
	if (m_watchArray[0].my_papi.num_events == 0) return;
    if (my_rank == 0) {
      fprintf(fp, "\n## PMlib Process Group [%5d] hardware performance counter (HWPC) Report ---\n", group);
    }

    for (int j = 0; j < m_nWatch; j++) {
      int i;
      if (op_sort == 0) {
        i = m_order[j]; //	0:経過時間順
      } else {
        i = j; //	1:登録順で表示
      }
      if (!m_watchArray[i].m_exclusive) continue;
      m_watchArray[i].printGroupHWPCsums(fp, m_watchArray[i].m_label, p_group, pp_ranks);
    }

#endif
  }


  /// MPI_Comm_splitで作成するグループ毎にMPIランク詳細レポートを出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] new_comm   MPI_Comm型 対応するcommunicator
  ///   @param[in] icolor int型 MPI_Comm_split()のカラー変数
  ///   @param[in] key    int型 MPI_Comm_split()のkey変数
  ///   @param[in] legend int型 (省略可) HWPC記号説明の表示(0:なし、1:表示する)
  ///   @param[in] op_sort (省略可)測定区間の表示順 (0:経過時間順、1:登録順)
  ///
  void PerfMonitor::printComm (FILE* fp, MPI_Comm new_comm, int icolor, int key, int legend, int op_sort)
  {

    if (!is_PMlib_enabled) return;

	int my_id, num_process;
	MPI_Group my_group;
	MPI_Comm_group(MPI_COMM_WORLD, &my_group);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &num_process);

	int ngroups;
	int *g_icolor;
	g_icolor = new int[num_process]();
	MPI_Gather(&icolor,1,MPI_INT, g_icolor,1,MPI_INT, 0, MPI_COMM_WORLD);

	#ifdef DEBUG_PRINT_MPI_GROUP
	(void) MPI_Barrier(MPI_COMM_WORLD);
	fprintf(stderr, "<printComm> MPI_Gather finished. my_id=%d, my_group=%d\n",
		my_id, my_group);
	#endif

	//	if(my_id != 0) return;

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
		PerfMonitor::printGroup (stdout, new_group, new_comm, p, i, 0, op_sort);
	}
	delete[] g_icolor ;
	delete[] g_myid ;
	delete[] p_gid  ;
	delete[] p_color ;
	delete[] p_size ;
  }


  /// 測定途中経過の状況レポートを出力（排他測定区間を対象とする）
  ///
  ///   @param[in] fp       出力ファイルポインタ
  ///   @param[in] comments 任意のコメント
  ///   @param[in] op_sort 測定区間の表示順 (0:経過時間順、1:登録順)
  ///
  ///   @note 基本レポートと同様なフォーマットで出力する。
  ///      MPIの場合、rank0プロセスの測定回数が１以上の区間のみを表示する。
  ///   @note  内部では以下の処理を行う。
  ///    各測定区間の全プロセス途中経過状況を集約。 gather_and_stats();
  ///    測定結果の平均値・標準偏差などの基礎的な統計計算。
  ///    経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
  ///    各測定区間のHWPCイベントの統計値を取得する。
  ///
  void PerfMonitor::printProgress(FILE* fp, std::string comments, int op_sort)
  {
    if (!is_PMlib_enabled) return;

    if (m_nWatch == 0) return; // No section defined with setProperties()

    gather_and_stats();

    sort_m_order();

    if (my_rank != 0) return;

    // 測定時間の分母
    //  排他測定区間の合計
    //  排他測定区間+非排他測定区間の合計
    double tot = 0.0;
    for (int i = 0; i < m_nWatch; i++) {
      if (m_watchArray[i].m_exclusive) {
        tot +=  m_watchArray[i].m_time_av;
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

    double sum_time_comm = 0.0;
    double sum_time_flop = 0.0;
    double sum_time_other = 0.0;
    double sum_comm = 0.0;
    double sum_flop = 0.0;
    double sum_other = 0.0;
    std::string unit;

    fprintf(fp, "\n### PMlib printProgress ---------- %s \n", comments.c_str());
    // 各測定区間を出力
    PerfMonitor::printBasicSections (fp, maxLabelLen, tot, sum_flop, sum_comm, sum_other,
                                  sum_time_flop, sum_time_comm, sum_time_other, unit, op_sort);

    return;
  }

  /// ポスト処理用traceファイルの出力と終了処理
  ///
  /// @note current version supports OTF(Open Trace Format) v1.5
  /// @note This API terminates producing post trace immediately, and may
  ///       produce non-pairwise start()/stop() records.
  ///
  void PerfMonitor::postTrace(void)
  {

    if (!is_PMlib_enabled) return;

    if (m_nWatch == 0) return; // No section defined with setProperties()

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
	  fprintf(stderr, "\t<postTrace> \n");
    }
	#endif

    gather_and_stats();

#ifdef USE_OTF
    // OTFファイルの出力と終了処理
    if (is_OTF_enabled) {
      std::string label;
      for (int i=0; i<m_nWatch; i++) {
        loop_section_object(i, label);
        m_watchArray[i].labelOTF (label, i);
      }
      m_watchArray[0].finalizeOTF();
    }
#endif

	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank == 0) {
	  fprintf(stderr, "\t<postTrace> ends\n");
    }
	#endif
  }


  /// 基本統計レポートのヘッダ部分を出力。
  ///
  ///   @param[in] fp       出力ファイルポインタ
  ///   @param[in] hostname ホスト名(省略時はrank 0 実行ホスト名)
  ///   @param[in] comments 任意のコメント
  ///   @param[in] tot 測定経過時間
  ///
  void PerfMonitor::printBasicHeader(FILE* fp, std::string hostname, const std::string comments, double tot)
  {

    if (!is_PMlib_enabled) return;

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

	// PMlibインストール時のサポートプログラムモデルについての情報を出力する
    fprintf(fp, "\n# PMlib Basic Report ------------------------------------------------------------- #\n");
    fprintf(fp, "\n");
    fprintf(fp, "\tPerformance Statistics Report from PMlib version %s\n", PM_VERSION);
    fprintf(fp, "\tLinked PMlib supports: ");
#ifdef DISABLE_MPI
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
#ifdef USE_POWER
    fprintf(fp, ", PowerAPI");
#else
    fprintf(fp, ", no-PowerAPI");
#endif
#ifdef USE_OTF
    fprintf(fp, ", OTF");
#else
    fprintf(fp, ", no-OTF");
#endif
    fprintf(fp, " on this system\n");

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

    m_watchArray[0].printEnvVars(fp);

    fprintf(fp, "\tActive PMlib elapse time (from initialize to report/print) = %9.3e [sec]\n", tot);
    fprintf(fp, "\tExclusive sections and inclusive sections are reported below.\n");
    fprintf(fp, "\tInclusive sections, marked with (*), are not added in the statistics total.\n");
    fprintf(fp, "\n");

  }


  /// 基本統計レポートの各測定区間を出力
  ///
  ///   @param[in] fp       出力ファイルポインタ
  ///   @param[in] maxLabelLen    ラベル文字長
  ///   @param[in] tot  Root区間の経過時間
  ///   @param[out] sum_time_flop  演算経過時間
  ///   @param[out] sum_time_comm  通信経過時間
  ///   @param[out] sum_time_other  その他経過時間
  ///   @param[out] sum_flop  演算量
  ///   @param[out] sum_comm  通信量
  ///   @param[out] sum_other  その他（% percentage で表示される量）
  ///   @param[in] unit 計算量の単位
  ///   @param[in] op_sort int型 測定区間の表示順 (0:経過時間順、1:登録順)
  ///
  ///   @note  計算量（演算数やデータ移動量）選択方法は PerfWatch::stop() のコメントに詳しく説明されている。
  ///
  void PerfMonitor::printBasicSections(FILE* fp, int maxLabelLen, double& tot,
                                  double& sum_flop, double& sum_comm, double& sum_other,
                                  double& sum_time_flop, double& sum_time_comm, double& sum_time_other,
                                  std::string unit, int op_sort)
  {
    if (!is_PMlib_enabled) return;

	// this member is call by rank 0 process only
	#ifdef DEBUG_PRINT_MONITOR
	if (my_rank == 0) {
		fprintf(stderr, "\n debug <printBasicSections> m_nWatch=%d\n",m_nWatch);
		fprintf(stderr, "\t address of m_order=%p\n", m_order);
	}
	#endif


    int is_unit;
    is_unit = m_watchArray[0].statsSwitch();
	//	fprintf(fp, "%-*s| number of|        accumulated time[sec]           ", maxLabelLen, "Section");
	fprintf(fp, "%-*s| number of|  averaged process execution time [sec] ", maxLabelLen, "Section");

    if ( is_unit == 0 || is_unit == 1 ) {
      fprintf(fp, "| user defined numerical performance\n");
    } else if ( is_unit == 2 ) {
      fprintf(fp, "| hardware counted data access events\n");
    } else if ( is_unit == 3 ) {
      fprintf(fp, "| hardware counted floating point ops.\n");
    } else if ( is_unit == 4 ) {
      fprintf(fp, "| hardware vectorized floating point ops.\n");
    } else if ( is_unit == 5 ) {
      fprintf(fp, "| hardware counted cache utilization\n");
    } else if ( is_unit == 6 ) {
      fprintf(fp, "| hardware counted total instructions\n");
    } else if ( is_unit == 7 ) {
      fprintf(fp, "| memory load and store instruction type\n");
    } else {
      fprintf(fp, "| *** internal bug. <printBasicSections> ***\n");
		;	// should not reach here
    }

	//	fprintf(fp, "%-*s|   calls  |      avr   avr[%%]     sdv    avr/call  ", maxLabelLen, "Label");
	fprintf(fp, "%-*s|   calls  |   total    [%%]   total/call     sdv    ", maxLabelLen, "Label");

    if ( is_unit == 0 || is_unit == 1 ) {
      fprintf(fp, "|  operations   sdv    performance\n");
    } else if ( is_unit == 2 ) {
      fprintf(fp, "|    Bytes      sdv   Mem+LLC bandwidth\n");
    } else if ( is_unit == 3 ) {
      fprintf(fp, "|  f.p.ops      sdv    performance\n");
    } else if ( is_unit == 4 ) {
      fprintf(fp, "|  f.p.ops      sdv    vectorized%%\n");
    } else if ( is_unit == 5 ) {
      fprintf(fp, "| load+store    sdv    L1+L2 hit%%\n");
    } else if ( is_unit == 6 ) {
      fprintf(fp, "| instructions  sdv    performance\n");
    } else if ( is_unit == 7 ) {
      fprintf(fp, "| load+store    sdv    vectorized%%\n");
    } else {
      fprintf(fp, "| *** internal bug. <printBasicSections> ***\n");
		;	// should not reach here
	}

	for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+--------------------------------\n");

    sum_time_comm = 0.0;
    sum_time_flop = 0.0;
    sum_time_other = 0.0;
    sum_comm = 0.0;
    sum_flop = 0.0;
    sum_other = 0.0;

    double fops;
    std::string p_label;

    double tav;

    // 測定区間の時間と計算量を表示。表示順は引数 op_sort で指定されている。
    for (int j = 0; j < m_nWatch; j++) {

      int i;
      if (op_sort == 0) {
        i = m_order[j]; //	0:経過時間順
      } else {
        i = j; //	1:登録順で表示
      }
      if (i == 0) continue;

      PerfWatch& w = m_watchArray[i];

      if ( !(w.m_count_sum > 0) ) continue;
      //	if ( !(w.m_count > 0) ) continue;
      //	if ( !w.m_exclusive || w.m_label.empty()) continue;

		//	tav = ( (w.m_count==0) ? 0.0 : w.m_time_av/w.m_count ); // 1回あたりの時間
		if (w.m_count_av != 0) {
			tav = w.m_time_av/(double)w.m_count_av;
		} else {
			tav = (double)num_process*w.m_time_av/(double)w.m_count_sum;
		}

      is_unit = w.statsSwitch();

      p_label = w.m_label;
      if (!w.m_exclusive) { p_label = w.m_label + "(*)"; }	// 非排他測定区間は単位表示が(*)

	//fprintf(fp, "%-*s|   calls  |   total    [%]    total/call     sdv    ", maxLabelLen, "Label");

      fprintf(fp, "%-*s: %8ld   %9.3e %6.2f  %9.3e  %8.2e",
              maxLabelLen,
              p_label.c_str(),
              w.m_count_av,         // 測定区間の平均コール回数 // w.m_count
              w.m_time_av,          // 測定区間の時間(全プロセスの平均値)
              100*w.m_time_av/tot,  // 比率：測定区間の時間/全区間（=Root区間）の時間
              tav,                 // コール1回あたりの時間
              w.m_time_sd          // 標準偏差
			);

		// 0: user set bandwidth
		// 1: user set flop counts
		// 2: BANDWIDTH : HWPC measured data access bandwidth
		// 3: FLOPS     : HWPC measured flop counts
		// 4: VECTOR    : HWPC measured vectorization (%)
		// 5: CACHE     : HWPC measured cache hit/miss (%)
		// 6: CYCLE     : HWPC measured cycles, instructions
		// 7: LOADSTORE : HWPC measured load/store instructions type (%)
      if (w.m_time_av == 0.0) {
        fops = 0.0;
      } else {
        if ( is_unit >= 0 && is_unit <= 1 ) {
          fops = (w.m_count_av==0) ? 0.0 : w.m_flop_av/w.m_time_av;
        } else
        if ( (is_unit == 2) || (is_unit == 3) || (is_unit == 6) ) {
          fops = (w.m_count_av==0) ? 0.0 : w.m_flop_av/w.m_time_av;
        } else
        if ( (is_unit == 4) || (is_unit == 5) || (is_unit == 7) ) {
          fops = w.m_percentage;
        }
      }

      double uF = w.unitFlop(fops, unit, is_unit);
      p_label = unit;		// 計算速度の単位
      if (!w.m_exclusive) { p_label = unit + "(*)"; } // 非排他測定区間は(*)表示

      fprintf(fp, "    %8.3e  %8.2e %6.2f %s\n",
            w.m_flop_av,          // 測定区間の計算量(全プロセスの平均値)
            w.m_flop_sd,          // 計算量の標準偏差(全プロセスの平均値)
            uF,                   // 測定区間の計算速度(全プロセスの平均値)
            p_label.c_str());		// 計算速度の単位

      if (w.m_exclusive) {
        if ( is_unit == 0 ) {
          sum_time_comm += w.m_time_av;
          sum_comm += w.m_flop_av;
        } else
        if ( is_unit == 1 ) {
          sum_time_flop += w.m_time_av;
          sum_flop += w.m_flop_av;
        } else
        if ( (is_unit == 2) || (is_unit == 3) || (is_unit == 6) ) {
          sum_time_flop += w.m_time_av;
          sum_flop += w.m_flop_av;

        } else
        if ( (is_unit == 4) || (is_unit == 5) || (is_unit == 7) ) {
          sum_time_flop += w.m_time_av;
          sum_flop += w.m_flop_av;
          sum_other += w.m_flop_av * uF;
        }

      }

    }	// for
  }


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
  void PerfMonitor::printBasicTailer(FILE* fp, int maxLabelLen,
                                     double sum_flop, double sum_comm, double sum_other,
                                     double sum_time_flop, double sum_time_comm, double sum_time_other,
                                     std::string unit)
  {

    if (!is_PMlib_enabled) return;

    int is_unit;
    is_unit = m_watchArray[0].statsSwitch();
	for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+--------------------------------\n");


    // Subtotal of the flop counts and/or byte counts per process
    //
    // 2021/6/14 Changed the label from  "Sections per process" to "All sections combined"
    //
    if ( (is_unit == 0) || (is_unit == 1) ) {
      if ( sum_time_comm > 0.0 ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "All sections combined", "", sum_time_comm);
      double comm_serial = PerfWatch::unitFlop(sum_comm/sum_time_comm, unit, 0);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive COMM sections-", sum_comm, comm_serial, unit.c_str());
      }
      if ( sum_time_flop > 0.0 ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "All sections combined", "", sum_time_flop);
      double flop_serial = PerfWatch::unitFlop(sum_flop/sum_time_flop, unit, 1);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive CALC sections-", sum_flop, flop_serial, unit.c_str());
      }
	} else
    if ( (is_unit == 2) || (is_unit == 3) || (is_unit == 6) ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "All sections combined", "", sum_time_flop);
      double flop_serial = PerfWatch::unitFlop(sum_flop/sum_time_flop, unit, is_unit);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive HWPC sections-", sum_flop, flop_serial, unit.c_str());

	} else
    if ( (is_unit == 4) || (is_unit == 5) || (is_unit == 7) ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "All sections combined", "", sum_time_flop);
      double other_serial = PerfWatch::unitFlop(sum_other/sum_flop, unit, is_unit);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive HWPC sections-", sum_flop, other_serial, unit.c_str());
	}


	for (int i = 0; i < maxLabelLen; i++) fputc('-', fp);
	fprintf(fp,       "+----------+----------------------------------------+--------------------------------\n");

    // Job total flop counts and/or byte counts
    //
    // 2021/6/14 Changed the label from  "Sections total job" to "Total of all processes"
    //
    if ( (is_unit == 0) || (is_unit == 1) ) {
      if ( sum_time_comm > 0.0 ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "Total of all processes", "", sum_time_comm);
      double sum_comm_job = (double)num_process*sum_comm;
      double comm_job = PerfWatch::unitFlop(sum_comm_job/sum_time_comm, unit, 0);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive COMM sections-", sum_comm_job, comm_job, unit.c_str());
      }
      if ( sum_time_flop > 0.0 ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "Total of all processes", "", sum_time_flop);
      double sum_flop_job = (double)num_process*sum_flop;
      double flop_job = PerfWatch::unitFlop(sum_flop_job/sum_time_flop, unit, 1);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive CALC sections-", sum_flop_job, flop_job, unit.c_str());
      }
	} else
    if ( (is_unit == 2) || (is_unit == 3) || (is_unit == 6) ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "Total of all processes", "", sum_time_flop);
      double sum_flop_job = (double)num_process*sum_flop;
      double flop_job = PerfWatch::unitFlop(sum_flop_job/sum_time_flop, unit, is_unit);
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive HWPC sections-", sum_flop_job, flop_job, unit.c_str());
	} else
    if ( (is_unit == 4) || (is_unit == 5) || (is_unit == 7) ) {
      fprintf(fp, "%-*s %1s %9.3e", maxLabelLen+10, "Total of all processes", "", sum_time_flop);
      double sum_flop_job = (double)num_process*sum_flop;
      double other_serial = PerfWatch::unitFlop(sum_other/sum_flop, unit, is_unit);
      double other_job = other_serial;
      fprintf(fp, "%30s  %8.3e          %7.2f %s\n", "-Exclusive HWPC sections-", sum_flop_job, other_job, unit.c_str());
	}

  }



// Extra Interface routine for Power API
//
//	PMlib C++  interface functions to simply monitor and controll the Power API library
//	current implementation is validated on supercomputer Fugaku
//
static void error_print(int , std::string , std::string);
static void warning_print (std::string , std::string , std::string );
static void warning_print (std::string , std::string , std::string , int);

void error_print(int irc, std::string cstr1, std::string cstr2) {
	fprintf(stderr, "*** PMlib Error. <power_ext::%s> failed. [%s] return code %d \n",
		cstr1.c_str(), cstr2.c_str(), irc);
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3) {
	fprintf(stderr, "*** PMlib Warning. <power_ext::%s> failed. [%s] %s \n",
		cstr1.c_str(), cstr2.c_str(), cstr3.c_str());
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3, int value) {
	fprintf(stderr, "*** PMlib Warning. <power_ext::%s> failed. [%s] %s : value %d \n",
		cstr1.c_str(), cstr2.c_str(), cstr3.c_str(), value);
	return;
}


int PerfMonitor::initializePOWER (void)
{
#ifdef USE_POWER

// Parse the Environment Variable POWER_CHOOSER
    level_POWER = 0;
	std::string s_chooser;
	char* cp_env = std::getenv("POWER_CHOOSER");
	if (cp_env == NULL) {
		;
	} else {
		s_chooser = cp_env;
		if (s_chooser == "OFF" || s_chooser == "NO" ) {
			level_POWER = 0;
		} else
		if (s_chooser == "NODE") {
			level_POWER = 1;
		} else
		if (s_chooser == "NUMA") {
			level_POWER = 2;
		} else
		if (s_chooser == "PARTS") {
			level_POWER = 3;
		}
	}
	#ifdef DEBUG_PRINT_POWER_EXT
    if (my_rank == 0) {
		fprintf(stderr, "<%s> POWER_CHOOSER=%s  level_POWER=%d \n", __func__, cp_env, level_POWER);
	}
	#endif
    if (level_POWER == 0) return(0);

	#ifdef DEBUG_PRINT_POWER_EXT
    if (my_rank == 0) {
	fprintf(stderr, "<%s> default objects. &pm_pacntxt=%p, &pm_obj_array=%p\n",
		__func__, &pm_pacntxt, &pm_obj_array[0]);
	}
	#endif

	int irc;
	int isum;

	isum = 0;
	isum += irc = PWR_CntxtInit (PWR_CNTXT_DEFAULT, PWR_ROLE_APP, "app", &pm_pacntxt);
	if (irc != PWR_RET_SUCCESS) error_print(irc, "PWR_CntxtInit", "default");

	for (int i=0; i<Max_power_object; i++) {
		isum += irc = PWR_CntxtGetObjByName (pm_pacntxt, p_obj_name[i], &pm_obj_array[i]);
		if (irc != PWR_RET_SUCCESS)
			{ warning_print ("CntxtGetObj", p_obj_name[i], "default"); continue;}
	}

	#ifdef DEBUG_PRINT_POWER_EXT
    if (my_rank == 0) {
	fprintf(stderr, "<%s> extended objects. &pm_extcntxt=%p, &pm_obj_ext=%p\n",
		__func__, &pm_extcntxt, &pm_obj_ext[0]);
	}
	#endif

	isum += irc = PWR_CntxtInit (PWR_CNTXT_FX1000, PWR_ROLE_APP, "app", &pm_extcntxt);
	if (irc != PWR_RET_SUCCESS) error_print(irc, "PWR_CntxtInit", "extended");

	for (int i=0; i<Max_power_extended; i++) {
		isum += irc = PWR_CntxtGetObjByName (pm_extcntxt, p_ext_name[i], &pm_obj_ext[i]);
		if (irc != PWR_RET_SUCCESS)
			{ warning_print ("CntxtGetObj", p_ext_name[i], "extended"); continue;}
	}

	if (isum == 0) {
		return (Max_power_object + Max_measure_device);	// =19+1=20
	} else {
		return(-1);
	}

#else
	return (0);
#endif
}


int PerfMonitor::finalizePOWER ()
{
#ifdef USE_POWER
    if (level_POWER == 0) return(0);

	#ifdef DEBUG_PRINT_POWER_EXT
    if (my_rank == 0) { fprintf(stderr, "\t <%s> CntxtDestroy()\n", __func__); }
	#endif

	int irc;
	irc = 0;
	irc = PWR_CntxtDestroy(pm_pacntxt);
	irc += PWR_CntxtDestroy(pm_extcntxt);

	#ifdef DEBUG_PRINT_POWER_EXT
    if (my_rank == 0) { fprintf(stderr, "\t <%s> returns %d\n", __func__, irc); }
	#endif
	return irc;
#else
	return (0);
#endif
}


int PerfMonitor::operatePowerKnob (int knob, int operation, int & value)
{
#ifdef USE_POWER
    if (level_POWER == 0) {
    	if (my_rank == 0) {
			fprintf(stderr, "*** Warning PMlib <%s> is ignored. Set POWER_CHOOSER to activate it. \n", __func__);
		}
		return(0);
	}

	//	knob and value combinations
	//		knob=0 : I_knob_CPU    : cpu frequency (MHz)
	//		knob=1 : I_knob_MEMORY : memory access throttling (%) : 100, 90, 80, .. , 10
	//		knob=2 : I_knob_ISSUE  : instruction issues (/cycle) : 2, 4
	//		knob=3 : I_knob_PIPE   : number of PIPEs : 1, 2
	//		knob=4 : I_knob_ECO    : eco state (mode) : 0, 1, 2
	//		knob=5 : I_knob_RETENTION : retention state (mode) : 0, 1
	//	operation : [0:read, 1:update]

	PWR_Obj *p_obj_array = pm_obj_array;
	PWR_Obj *p_obj_ext   = pm_obj_ext;
	uint64_t u64array[Max_power_leaf_parts];
	uint64_t u64;

	PWR_Grp p_obj_grp = NULL;
	int irc;
	const int reading_operation=0;
	const int update_operation=1;
	double Hz;
		int j;

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "<operatePowerKnob> knob=%d, operation=%d, value=%d\n",
		knob, operation, value);
	#endif

	if ( knob < 0 | knob >Max_power_knob ) {
		error_print(knob, "operatePowerKnob", "invalid controler");
		return(-1);
	}
	if ( operation == update_operation ) {
		for (int i=0; i < Max_power_leaf_parts; i++) {
			u64array[i] = value;
		}
	}

	if ( knob == I_knob_CPU ) {
		// CPU frequency can be controlled either from the default context or the extended context
		if ( operation == reading_operation ) {
			irc = PWR_ObjAttrGetValue (p_obj_ext[I_pext_CPU], PWR_ATTR_FREQ, &Hz, NULL);
			if (irc != PWR_RET_SUCCESS)
				{ error_print(irc, "GetValue", p_ext_name[I_pext_CPU]); return(irc); }
			value = lround (Hz / 1.0e6);
		} else {
			if ( !(value == 2200 || value == 2000) ) {	// 1.6 GHz retention is not allowed
				warning_print ("SetValue", p_ext_name[I_pext_CPU], "invalid frequency", value);
				return(-1);
			}
			Hz = (double)value * 1.0e6;
			irc = PWR_ObjAttrSetValue (p_obj_ext[I_pext_CPU], PWR_ATTR_FREQ, &Hz);
			if (irc != PWR_RET_SUCCESS)
				{ error_print(irc, "SetValue", p_ext_name[I_pext_CPU]); return(irc); }
		}
	} else
	if ( knob == I_knob_MEMORY ) {
		// Memory throttling from the extended context
		for (int icmg=0; icmg <4 ; icmg++)
		{
			j=I_pext_MEM0+icmg;
			if ( operation == reading_operation ) {
				irc = PWR_ObjAttrGetValue (p_obj_ext[j], PWR_ATTR_THROTTLING_STATE, &u64, NULL);
				if (irc != PWR_RET_SUCCESS)
					{ error_print(irc, "GetValue", p_ext_name[j]); return(irc); }
				value = u64;
			} else {
				if ( !(0 <= value && value <= 9) ) {
					warning_print ("SetValue", p_ext_name[j], "invalid throttling", value);
					return(-1);
				}
				u64 = value;
				irc = PWR_ObjAttrSetValue (p_obj_ext[j], PWR_ATTR_THROTTLING_STATE, &u64);
				if (irc != PWR_RET_SUCCESS)
					{ error_print(irc, "SetValue", p_ext_name[j]); return(irc); }
			}
		}

	} else
	if ( knob == I_knob_ISSUE ) {
		// sub group ISSUE control from the extended context
		for (int icmg=0; icmg <4 ; icmg++)
		{
			p_obj_grp = NULL;
			j=I_pext_CMG0CORES+icmg;
			irc = PWR_ObjGetChildren (p_obj_ext[j], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS)
					{ error_print(irc, "GetChildren", p_ext_name[j]); return(irc); }
			if ( operation == reading_operation ) {
				irc = PWR_GrpAttrGetValue (p_obj_grp, PWR_ATTR_ISSUE_STATE, u64array, NULL, NULL);
				(void) PWR_GrpDestroy (p_obj_grp);
				if (irc != PWR_RET_SUCCESS)
						{ error_print(irc, "GrpGet (ISSUE)",  p_ext_name[j]); return(irc); }
				value = (int)u64array[0];
			} else {
				irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_ISSUE_STATE, u64array, NULL);
				(void) PWR_GrpDestroy (p_obj_grp);
				if (irc != PWR_RET_SUCCESS)
						{ error_print(irc, "GrpSet (ISSUE)",  p_ext_name[j]); return(irc); }
			}
		}

	} else
	if ( knob == I_knob_PIPE ) {
		// sub group PIPE control from the extended context
		for (int icmg=0; icmg <4 ; icmg++)
		{
			p_obj_grp = NULL;
			j=I_pext_CMG0CORES+icmg;
			irc = PWR_ObjGetChildren (p_obj_ext[j], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS)
					{ error_print(irc, "GetChildren", p_ext_name[j]); return(irc); }

			if ( operation == reading_operation ) {
				irc = PWR_GrpAttrGetValue (p_obj_grp, PWR_ATTR_EX_PIPE_STATE, u64array, NULL, NULL);
				(void) PWR_GrpDestroy (p_obj_grp);
				if (irc != PWR_RET_SUCCESS)
						{ error_print(irc, "GrpGet (PIPE)", p_ext_name[j]); return(irc); }
				value = (int)u64array[0];
			} else {
				irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_EX_PIPE_STATE, u64array, NULL);
				(void) PWR_GrpDestroy (p_obj_grp);
				if (irc != PWR_RET_SUCCESS)
						{ error_print(irc, "GrpSet (PIPE)", p_ext_name[j]); return(irc); }
			}
		}

	} else
	if ( knob == I_knob_ECO ) {
		// sub group ECO control from the extended context
		for (int icmg=0; icmg <4 ; icmg++)
		{
			p_obj_grp = NULL;
			j=I_pext_CMG0CORES+icmg;
			irc = PWR_ObjGetChildren (p_obj_ext[j], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS)
					{ error_print(irc, "GetChildren", p_ext_name[j]); return(irc); }

			if ( operation == reading_operation ) {
				irc = PWR_GrpAttrGetValue (p_obj_grp, PWR_ATTR_ECO_STATE, u64array, NULL, NULL);
				irc = PWR_GrpDestroy (p_obj_grp);
				if (irc != PWR_RET_SUCCESS)
						{ error_print(irc, "GrpGet (ECO)", p_ext_name[j]); return(irc); }
				value = (int)u64array[0];
			} else {
				irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_ECO_STATE, u64array, NULL);
				irc = PWR_GrpDestroy (p_obj_grp);
				if (irc != PWR_RET_SUCCESS)
						{ error_print(irc, "GrpSet (ECO)", p_ext_name[j]); return(irc); }
			}
		}

	/**
	// RETENTION is disabled
	} else
	if ( knob == I_knob_RETENTION ) {
		warning_print ("operatePowerKnob", "RETENTION", "user RETENTION control is not allowed");
		return(-1);
	**/
	} else {
		//	should not reach here
		error_print(knob, "operatePowerKnob", "internal error. knob="); return(-1);
		;
	}

	return(0);

#else
	return (0);
#endif
}


  /// 測定区間のラベルに対応する区間番号を追加作成する
  /// Add a new entry in the section map (section name, section ID)
  ///
  ///   @param[in] arg_st   the label of the newly created section
  ///
int PerfMonitor::add_section_object(std::string arg_st)
{
	int mid;
	mid = m_nWatch;
   	m_map_sections.insert( make_pair(arg_st, mid) );

    #ifdef DEBUG_PRINT_LABEL
	if (my_rank==0) {
	fprintf(stderr, "<add_section_object> [%s] [%d] my_thread=%d \n", arg_st.c_str(), mid, my_thread);
	}
    #endif
   	// we may better return the insert status...?
	return mid;
}

  /// 測定区間のラベルに対応する区間番号を取得
  /// Search through the map and find section ID from the given label
  ///
  ///   @param[in] arg_st   the label of the section
  ///
int PerfMonitor::find_section_object(std::string arg_st)
{
   	int mid;
   	if (m_map_sections.find(arg_st) == m_map_sections.end()) {
   		mid = -1;
   	} else {
   		mid = m_map_sections[arg_st] ;
   	}
	#ifdef DEBUG_PRINT_LABEL
	if (my_rank==0) {
   	fprintf(stderr, "<find_section_object> %s : mid=%d my_thread=%d \n", arg_st.c_str(), mid, my_thread);
	}
	#endif
   	return mid;
}

  /// 測定区間の区間番号に対応するラベルを取得
  /// Search the section ID in the map and return the label string
  ///
  ///   @param[in]  mid   	the section ID
  ///   @param[out] p_label   the label string of the section
  ///
void PerfMonitor::loop_section_object(const int mid, std::string& p_label)
{
	std::map<std::string, int>::const_iterator it;
	int p_id;

	for(it = m_map_sections.begin(); it != m_map_sections.end(); ++it) {
		if (it->second == mid) {
			p_label = it->first;
			#ifdef DEBUG_PRINT_LABEL
			if (my_rank==0) {
			fprintf(stderr, "<loop_section_object> mid=%d my_thread=%d matched to [%s] \n", mid, my_thread, p_label.c_str() );
			}
			#endif
			return;
		}
	}
	// should not reach here
	fprintf(stderr, "*** Error PMlib <loop_section_object> section ID %d was not found. my_rank=%d, my_thread=%d \n", mid, my_rank, my_thread);
}

  /// 全測定区間のラベルと番号を登録順で表示
  /// Check print all the defined section IDs and labels
  ///
void PerfMonitor::check_all_section_object(void)
{
	std::map<std::string, int>::const_iterator it;
	std::string p_label;
	int p_id;
	int n;
	n = m_map_sections.size();
	fprintf(stderr, "<check_all_section_object> map size=%d \n", n);
	if (n==0) return;
	fprintf(stderr, "\t[map pair] : label, value, &(it->first), &(it->second)\n");
	for(it = m_map_sections.begin(); it != m_map_sections.end(); ++it) {
		p_label = it->first;
		p_id = it->second;
		fprintf(stderr, "\t <%s> : %d, %p, %p\n", p_label.c_str(), p_id, &(it->first), &(it->second));

	}
}

  /// Add a new entry in the shared section map (section name, section ID)
  ///
  ///   @param[in] arg_st   the label of the newly created shared section
  ///
  ///	@note The shared map is accessible from all threads inside or outside of parallel construct
  ///
int PerfMonitor::add_shared_section(std::string arg_st)
{
   	int n_shared_sections;
	int n = shared_map_sections.size();
		//	int ip = m_nWatch; // was a BUG
	#ifdef _OPENMP
	#pragma omp critical
	// remark. end critical does not exist for C++. its only for fortran !$omp.
	#endif
	{
   		shared_map_sections.insert( make_pair(arg_st, n) );
   		n_shared_sections = shared_map_sections[arg_st] ;

    	#ifdef DEBUG_PRINT_LABEL
		if (my_rank==0) {
		fprintf(stderr, "<add_shared_section> [%s] [%d] my_thread=%d \n", arg_st.c_str(), n_shared_sections, my_thread);
		}
    	#endif
	}
	return n_shared_sections;
}

  /// Print all the shared section IDs and labels 
  ///
void PerfMonitor::check_all_shared_sections(void)
{
	std::map<std::string, int>::const_iterator it;
	std::string p_label;
	int p_id;
	int n_shared_sections = shared_map_sections.size();
	fprintf(stderr, "<check_all_shared_sections> shared map size=%d \n", n_shared_sections);
	if (n_shared_sections==0) return;

	fprintf(stderr, "\t[map pair] : label, value, &(it->first), &(it->second)\n");
	for(it = shared_map_sections.begin(); it != shared_map_sections.end(); ++it) {
		p_label = it->first;
		p_id = it->second;
		fprintf(stderr, "\t [%s] : %d, %p, %p\n", p_label.c_str(), p_id, &(it->first), &(it->second));

	}
}

} /* namespace pm_lib */

