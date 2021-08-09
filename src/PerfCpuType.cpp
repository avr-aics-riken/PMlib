/*
###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2020 RIKEN Center for Computational Science(R-CCS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2020 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################
 */

//@file   PerfCpuType.cpp
//@brief  PMlib - PAPI interface class

// if USE_PAPI is defined, compile this file with openmp option

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

#ifdef DISABLE_MPI
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif
#include <cmath>
#include "PerfWatch.h"

namespace pm_lib {

  extern struct pmlib_papi_chooser papi;
  extern struct hwpc_group_chooser hwpc_group;


  /// HWPC interface initialization
  ///
  /// @brief  PMlib - HWPC PAPI interface class
  /// PAPI library is used to interface HWPC events
  /// this routine is called directly by PerfMonitor class instance
  ///
  /// @note
  ///	Extern variables in thread parallel call.
  ///	When called from inside the parallel region, all threads
  ///	share the same address for external "papi" structure.
  ///	So, the variables such as papi.num_events show the same value.
  ///	The local variable have different address.
  ///
  /// @note
  ///	Class variables in thread parallel call.
  ///	If a class member is called from inside parallel region,
  ///	the variables of the class are given different addresses.
  ///
void PerfWatch::initializeHWPC ()
{

	bool root_in_parallel;
	int root_thread;

	root_in_parallel = false;
	root_thread = 0;
	#ifdef _OPENMP
	root_in_parallel = omp_in_parallel();
	root_thread = omp_get_thread_num();
	#endif

	#ifdef DEBUG_PRINT_PAPI
	if (my_rank == 0) {
		fprintf(stderr, "<initializeHWPC> master process thread %d, address of papi=%p, my_papi=%p\n", root_thread, &papi, &my_papi);
	}
	#endif

	if (root_thread == 0)
	{
	for (int i=0; i<Max_hwpc_output_group; i++) {
		hwpc_group.number[i] = 0;
		hwpc_group.index[i] = -999999;
		}

	papi.num_events = 0;
	for (int i=0; i<Max_chooser_events; i++){
		papi.events[i] = 0;
		papi.values[i] = 0;
		papi.accumu[i] = 0;
		papi.v_sorted[i] = 0;
		}
	for (int j=0; j<Max_nthreads; j++){
	for (int i=0; i<Max_chooser_events; i++){
			papi.th_values[j][i] = 0;
			papi.th_accumu[j][i] = 0;
			papi.th_v_sorted[j][i] = 0;
		}
		}
	}

// Parse the Environment Variable HWPC_CHOOSER
	std::string s_chooser;
	std::string s_default = "USER";
	char* cp_env = std::getenv("HWPC_CHOOSER");
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
			s_chooser = s_default;
		}
	}
	hwpc_group.env_str_hwpc = s_chooser;

	read_cpu_clock_freq(); /// API for reading processor clock frequency.

	if (hwpc_group.env_str_hwpc == "USER" ) return;	// Is this a correct return?

#ifdef USE_PAPI
	int i_papi;
	if (root_thread == 0)
	{
	i_papi = PAPI_library_init( PAPI_VER_CURRENT );
	if (i_papi != PAPI_VER_CURRENT ) {
		fprintf (stderr, "*** error. <PAPI_library_init> code: %d\n", i_papi);
		fprintf (stderr, "\t Check if correct version of PAPI library is linked.");
		PM_Exit(0);
		//	return;
		}

	i_papi = PAPI_thread_init( (unsigned long (*)(void)) (omp_get_thread_num) );
	if (i_papi != PAPI_OK ) {
		fprintf (stderr, "*** error. <PAPI_thread_init> failed. code=%d, root_thread=%d\n", i_papi, root_thread);
		PM_Exit(0);
		//	return;
		}

	createPapiCounterList ();

	#ifdef DEBUG_PRINT_PAPI
	if (my_rank == 0 && root_thread == 0) {
		fprintf(stderr, "<initializeHWPC> created struct papi: papi.num_events=%d, address=%p\n",
			papi.num_events, &papi.num_events );
		fprintf(stderr, "\t\t memo: struct papi is shared across threads.\n");
	}
	#endif
	}

	#pragma omp barrier
	// In general the arguments to <my_papi_*> should be thread private.
	// For APIs whose arguments do not change,  we use shared object.

	if (root_in_parallel) {
	int t_papi;
	t_papi = my_papi_add_events (papi.events, papi.num_events);
	if ( t_papi != PAPI_OK ) {
		fprintf(stderr, "*** error. <initializeHWPC> <my_papi_add_events> code: %d\n"
			"\n\t most likely un-supported HWPC PAPI combination.\n", t_papi);
		papi.num_events = 0;
		PM_Exit(0);
		}
	t_papi = my_papi_bind_start (papi.values, papi.num_events);
	if ( t_papi != PAPI_OK ) {
		fprintf(stderr, "*** error. <initializeHWPC> <my_papi_bind_start> code: %d\n", t_papi);
		PM_Exit(0);
		}

	} else {
	#pragma omp parallel
	{
	int t_papi;
	t_papi = my_papi_add_events (papi.events, papi.num_events);
	if ( t_papi != PAPI_OK ) {
		fprintf(stderr, "*** error. <initializeHWPC> <my_papi_add_events> code: %d\n"
			"\n\t most likely un-supported HWPC PAPI combination.\n", t_papi);
		papi.num_events = 0;
		PM_Exit(0);
		}
	t_papi = my_papi_bind_start (papi.values, papi.num_events);
	if ( t_papi != PAPI_OK ) {
		fprintf(stderr, "*** error. <initializeHWPC> <my_papi_bind_start> code: %d\n", t_papi);
		PM_Exit(0);
		}
	} // end of #pragma omp parallel
	} // end of if (root_in_parallel)

#endif // USE_PAPI
}



  /// Construct the available list of PAPI counters for the targer processor
  /// @note  this routine needs the processor hardware information
  ///
  /// this routine is called by PerfWatch::initializeHWPC() only once
  ///
void PerfWatch::createPapiCounterList ()
{
#ifdef USE_PAPI
// Set PAPI counter events. The events are CPU hardware dependent

	const PAPI_hw_info_t *hwinfo = NULL;
	using namespace std;
	std::string s_model_string;
	std::string s_vendor_string;
	int i_papi;

// 1. Identify the CPU architecture

	// Verified on the following platform
	//	corot:	: Intel(R) Core(TM) i5-3470S CPU @ 2.90GHz	# Ivybridge	2012 model
	//	star:	: Intel(R) Core(TM)2 Duo CPU     E7500  @ 2.93GHz, has sse4	# 2009 model
	// vsh-vsp	: Intel(R) Xeon(R) CPU E5-2670 0 @ 2.60GHz	# Sandybridge
	//	eagles:	: Intel(R) Xeon(R) CPU E5-2650 v2 @ 2.60GHz	# SandyBridge
	//	uv01:	: Intel(R) Xeon(R) CPU E5-4620 v2 @ 2.60GHz	# Ivybridge
	//	c01:	: Intel(R) Xeon(R) CPU E5-2640 v3 @ 2.60GHz	# Haswell
	// chicago:	: Intel(R) Xeon(R) CPU E3-1220 v5 @ 3.00GHz (94)# Skylake
	//  ito:	: Intel(R) Xeon(R) Gold 6154 CPU @ 3.00GHz	# Skylake
	// ito-fep:	: Intel(R) Xeon(R) CPU E7-8880 v4 @ 2.20GHz	# Broadwell
	// water:	: Intel(R) Xeon(R) Gold 6148 CPU @ 2.40GHz	# Skylake
	// fugaku:	: Fujitsu A64FX based on ARM SVE edition @ 2.0 GHz base frequency

	hwinfo = PAPI_get_hardware_info();
	if (hwinfo == NULL) {
		if (my_rank == 0) {
		fprintf (stderr, "*** PMlib error. <createPapiCounterList> PAPI_get_hardware_info() failed.\n" );
		fprintf (stderr, "\t check if the program is linked with the correct libpapi and libpfm .\n" );
		}
	}

	#ifdef DEBUG_PRINT_PAPI
	//	if (my_rank == 0 && root_thread == 0) {
	if (my_rank == 0) {
		fprintf(stderr, "<PerfWatch::createPapiCounterList> called PAPI_get_hardware_info()\n");
		fprintf(stderr, "vendor=%d, vendor_string=%s, model=%d, model_string=%s \n",
				hwinfo->vendor, hwinfo->vendor_string, hwinfo->model, hwinfo->model_string );
	}
	#endif

//	with Linux, s_model_string is usually taken from "model name" in /proc/cpuinfo
	s_model_string = hwinfo->model_string;
	s_vendor_string = hwinfo->vendor_string;

	// Intel Xeon processors
    if (s_model_string.find( "Intel" ) != string::npos) {
		// hwpc_group.i_platform = 0;	// un-supported platform
		// hwpc_group.i_platform = 1;	// Minimal support for only two types
		// hwpc_group.i_platform = 2;	// Sandybridge and alike platform
		// hwpc_group.i_platform = 3;	// Haswell does not support FLOPS events
		// hwpc_group.i_platform = 4;	// Broadwell. No access to Broadwell yet.
		// hwpc_group.i_platform = 5;	// Skylake and alike platform

    	if (s_model_string.find( "Xeon" ) != string::npos) {
			hwpc_group.platform = "Xeon" ;

		// parse s_model_string
    	if (s_model_string.find( "E3" ) != string::npos ||
			s_model_string.find( "E5" ) != string::npos ||
			s_model_string.find( "E7" ) != string::npos ) {
			hwpc_group.i_platform = 2;	// Xeon default model : Sandybridge and Ivybridge

			if (s_model_string.find( "v3" ) != string::npos ) {
				hwpc_group.i_platform = 3;	// Haswell
			} else if (s_model_string.find( "v4" ) != string::npos ) {
				hwpc_group.i_platform = 4;	// Broadwell
			} else if (s_model_string.find( "v5" ) != string::npos ) {
				hwpc_group.i_platform = 5;	// Skylake without avx-512
			}
		}
    	else if (s_model_string.find( "Platinum" ) != string::npos ||
			s_model_string.find( "Gold" ) != string::npos ||
			s_model_string.find( "Silver" ) != string::npos ) {
			hwpc_group.i_platform = 5;	// Skylake
		}

		} else
		if (s_model_string.find( "Core(TM)" ) != string::npos ) {
			if (s_model_string.find( "i5-" ) != string::npos ) {
				hwpc_group.platform = "Xeon" ;
				hwpc_group.i_platform = 2;	// Ivybridge
    		} else {
				hwpc_group.platform = "Xeon" ;
				hwpc_group.i_platform = 1;	// Minimal support. only two FLOPS types
			}
		}
    	else {
			hwpc_group.i_platform = 0;	// un-supported Xeon type
		}

		// further parse s_model_string to find out coreGHz
		std::string s_separator = "@", s_terminator = "GHz";
		int loc_ATmark, loc_GHz, nchars;
		loc_ATmark = s_model_string.find_first_of(s_separator);
		loc_GHz    = s_model_string.find(s_terminator, loc_ATmark);
		hwpc_group.coreGHz = stod ( s_model_string.substr(loc_ATmark+1, loc_GHz-loc_ATmark-1) );
		if ( hwpc_group.coreGHz <= 1.0 ||  hwpc_group.coreGHz >= 10.0 ) {
			hwpc_group.coreGHz = 1.000;
		}

		if (hwpc_group.i_platform == 1) {
			hwpc_group.corePERF = hwpc_group.coreGHz * 1.0e9 * 1;
		} else if (hwpc_group.i_platform == 2) {
			hwpc_group.corePERF = hwpc_group.coreGHz * 1.0e9 * 8;
		} else if (hwpc_group.i_platform == 3) {
			hwpc_group.corePERF = hwpc_group.coreGHz * 1.0e9 * 8;
		} else if (hwpc_group.i_platform == 4) {
			hwpc_group.corePERF = hwpc_group.coreGHz * (2.7/3.0) * 1.0e9 * 16;
		} else if (hwpc_group.i_platform == 5) {
			hwpc_group.corePERF = hwpc_group.coreGHz * (2.7/3.0) * 1.0e9 * 32;
		}
	}


	// SPARC based processors
    else if (s_model_string.find( "SPARC64" ) != string::npos ) {
		hwpc_group.platform = "SPARC64" ;
    	if ( s_model_string.find( "VIIIfx" ) != string::npos ) {
			hwpc_group.i_platform = 8;	// K computer
			hwpc_group.coreGHz = 2.0;
			hwpc_group.corePERF = hwpc_group.coreGHz * 1.0e9 * 8;
		}
    	else if ( s_model_string.find( "IXfx" ) != string::npos ) {
			hwpc_group.i_platform = 9;	// Fujitsu FX10
			hwpc_group.coreGHz = 1.848;
			hwpc_group.corePERF = hwpc_group.coreGHz * 1.0e9 * 8;
		}
    	else if ( s_model_string.find( "XIfx" ) != string::npos ) {
			hwpc_group.i_platform = 11;	// Fujitsu FX100
			hwpc_group.coreGHz = 1.975;
			hwpc_group.corePERF = hwpc_group.coreGHz * 1.0e9 * 16;
		}
	}

	// ARM based processors
    else if (s_model_string.empty() && s_vendor_string.find( "ARM" ) != string::npos ) {
		// on ARM, PAPI_get_hardware_info() does not provide so useful information.
		// so we check /proc/cpuinfo for further information
		//
		identifyARMplatform ();

    	if ( hwpc_group.i_platform == 21 ) {
			hwpc_group.platform = "A64FX" ;
			s_model_string = hwpc_group.platform ;
			hwpc_group.coreGHz = 2.0;
			hwpc_group.corePERF = hwpc_group.coreGHz * 1.0e9 * 32;
		} else {
			//	hwpc_group.i_platform = 99;
			hwpc_group.platform = "unsupported_hardware";
			s_model_string = hwpc_group.platform ;
		}

	}

	// other processors are not supported by PMlib
    else {
			hwpc_group.i_platform = 99;
			hwpc_group.platform = "unsupported_hardware";
	}


// 2. Parse the Environment Variable HWPC_CHOOSER
//	the parsing has been done in initializeHWPC() routine


// 3. Select the corresponding PAPI hardware counter events
	int ip=0;


// if (FLOPS)
	if ( hwpc_group.env_str_hwpc == "FLOPS" ) {
		hwpc_group.index[I_flops] = ip;
		hwpc_group.number[I_flops] = 0;

		if (hwpc_group.platform == "Xeon" ) { // Intel Xeon
			if (hwpc_group.i_platform == 1 ||
				hwpc_group.i_platform == 2 ||
				hwpc_group.i_platform == 4 ||
				hwpc_group.i_platform == 5 ) {
				hwpc_group.number[I_flops] += 2;
				papi.s_name[ip] = "SP_OPS"; papi.events[ip] = PAPI_SP_OPS; ip++;
				papi.s_name[ip] = "DP_OPS"; papi.events[ip] = PAPI_DP_OPS; ip++;
			} else {
				;	// hwpc_group.i_platform = 3;	// Haswell does not support FLOPS events
			}
		} else
		if (hwpc_group.platform == "SPARC64" ) {
			if (hwpc_group.i_platform == 8 ||
				hwpc_group.i_platform == 9 ||
				hwpc_group.i_platform == 11 ) {
				hwpc_group.number[I_flops] += 1;
				papi.s_name[ip] = "FP_OPS"; papi.events[ip] = PAPI_FP_OPS; ip++;
				//	single precision count is not supported on FX100
			}
		} else

		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
				hwpc_group.number[I_flops] += 2;
				papi.s_name[ip] = "SP_OPS"; papi.events[ip] = PAPI_SP_OPS; ip++;
				papi.s_name[ip] = "DP_OPS"; papi.events[ip] = PAPI_DP_OPS; ip++;
			}
		}
	}

	else
// if (BANDWIDTH)
	if ( hwpc_group.env_str_hwpc == "BANDWIDTH" ) {
		hwpc_group.index[I_bandwidth] = ip;
		hwpc_group.number[I_bandwidth] = 0;

		if (hwpc_group.platform == "Xeon" ) {
			// the data transactions from/to memory edge device, i.e. outside Last Level Cache,
			// can be directly obtained with Intel PMU API, but not with PAPI.
			// We choose to use uncore event counter to calculate the memory bandwidth
			// The uncore event naming changed over the PAPI versions and over Xeon generation
			// We do not count RFo events since adding more events cause errors/overflows.

			// data feed from L2 cache
			if (hwpc_group.i_platform >= 2 ) {
				hwpc_group.number[I_bandwidth] += 2;
				papi.s_name[ip] = "L2_RQSTS:DEMAND_DATA_RD_HIT";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_RD_HIT"; ip++;
				papi.s_name[ip] = "L2_RQSTS:PF_HIT";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_PF_HIT"; ip++;
			}

			// data feed from L3 cache and from memory
			if (hwpc_group.i_platform == 2 ) {
				hwpc_group.number[I_bandwidth] += 2;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:LLC_HITMESF";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_HIT"; ip++;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:L3_MISS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_MISS"; ip++;
			} else
			if (hwpc_group.i_platform == 3 ) {
				hwpc_group.number[I_bandwidth] += 2;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:L3_HIT";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_HIT"; ip++;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:L3_MISS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_MISS"; ip++;
			} else
			if (hwpc_group.i_platform == 4 ) {
				hwpc_group.number[I_bandwidth] += 2;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:SPL_HIT";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_HIT"; ip++;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:L3_MISS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_MISS"; ip++;
			} else
			if (hwpc_group.i_platform == 5 ) {
				hwpc_group.number[I_bandwidth] += 2;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:L3_HIT";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_HIT"; ip++;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:ANY_DATA:L3_MISS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L3_MISS"; ip++;
			}

		} else
		if (hwpc_group.platform == "SPARC64" ) {
			hwpc_group.number[I_bandwidth] += 6;
			papi.s_name[ip] = "L2_READ_DM";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "L2_READ_PF";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;

			//	SPARC64 event PAPI_L2_TCM == (L2_MISS_DM + L2_MISS_PF)
			papi.s_name[ip] = "L2_MISS_DM";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "L2_MISS_PF";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;

			// The following two events are not shown from papi_avail -d command.
			// They show up from papi_event_chooser NATIVE L2_WB_DM (or L2_WB_PF) command.
			papi.s_name[ip] = "L2_WB_DM";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "L2_WB_PF";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;

		} else
		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
			// On A64FX, we use native events BUS_READ_TOTAL_MEM and BUS_WRITE_TOTAL_MEM
			hwpc_group.number[I_bandwidth] += 2;
			papi.s_name[ip] = "BUS_READ_TOTAL_MEM";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "CMG_bus_RD"; ip++;
			papi.s_name[ip] = "BUS_WRITE_TOTAL_MEM";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "CMG_bus_WR"; ip++;

			}
		}

	}

	else
// if (VECTOR)
	if ( hwpc_group.env_str_hwpc == "VECTOR" ) {
		hwpc_group.index[I_vector] = ip;
		hwpc_group.number[I_vector] = 0;

		if (hwpc_group.platform == "Xeon" ) {
		// Remark : VECTOR option for Xeon is available for Sandybridge-EP and newer CPUs only.

			if (hwpc_group.i_platform == 1 ) {
				// Basic support for two types only
				hwpc_group.number[I_vector] += 2;
				papi.s_name[ip] = "SP_OPS"; papi.events[ip] = PAPI_SP_OPS; ip++;
				papi.s_name[ip] = "DP_OPS"; papi.events[ip] = PAPI_DP_OPS; ip++;
					//	PAPI_FP_OPS (=PAPI_FP_INS) is not useful on Xeon. un-packed operations only.
			} else
			if ( hwpc_group.i_platform == 2 ) {
				// Sandybridge v2 and alike platform
				hwpc_group.number[I_vector] += 6;
				papi.s_name[ip] = "FP_COMP_OPS_EXE:SSE_FP_SCALAR_SINGLE";		// scalar
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_SINGLE"; ip++;
				papi.s_name[ip] = "FP_COMP_OPS_EXE:SSE_PACKED_SINGLE";		// 4 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_SSE"; ip++;
				papi.s_name[ip] = "SIMD_FP_256:PACKED_SINGLE";				// 8 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_AVX"; ip++;
				papi.s_name[ip] = "FP_COMP_OPS_EXE:SSE_SCALAR_DOUBLE";		// scalar
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_SINGLE"; ip++;
				papi.s_name[ip] = "FP_COMP_OPS_EXE:SSE_FP_PACKED_DOUBLE";	// 2 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_SSE"; ip++;
				papi.s_name[ip] = "SIMD_FP_256:PACKED_DOUBLE";				// 4 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_AVX"; ip++;
			} else
			if ( hwpc_group.i_platform == 4 ) {
				// Broadwell
				hwpc_group.number[I_vector] += 6;
				papi.s_name[ip] = "FP_ARITH:SCALAR_SINGLE";			//	scalar
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_SINGLE"; ip++;
				papi.s_name[ip] = "FP_ARITH:128B_PACKED_SINGLE";	//	4 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_SSE"; ip++;
				papi.s_name[ip] = "FP_ARITH:256B_PACKED_SINGLE";	//	8 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_AVX"; ip++;
				papi.s_name[ip] = "FP_ARITH:SCALAR_DOUBLE";			//	scalar
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_SINGLE"; ip++;
				papi.s_name[ip] = "FP_ARITH:128B_PACKED_DOUBLE";	//	2 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_SSE"; ip++;
				papi.s_name[ip] = "FP_ARITH:256B_PACKED_DOUBLE";	//	4 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_AVX"; ip++;
			} else
			if ( hwpc_group.i_platform == 5 ) {
				// Skylake and alike platform
				hwpc_group.number[I_vector] += 8;
				papi.s_name[ip] = "FP_ARITH:SCALAR_SINGLE";			//	scalar
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_SINGLE"; ip++;
				papi.s_name[ip] = "FP_ARITH:128B_PACKED_SINGLE";	//	4 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_SSE"; ip++;
				papi.s_name[ip] = "FP_ARITH:256B_PACKED_SINGLE";	//	8 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_AVX"; ip++;
				papi.s_name[ip] = "FP_ARITH:512B_PACKED_SINGLE";	//	16 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_AVXW"; ip++;
				papi.s_name[ip] = "FP_ARITH:SCALAR_DOUBLE";			//	scalar
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_SINGLE"; ip++;
				papi.s_name[ip] = "FP_ARITH:128B_PACKED_DOUBLE";	//	2 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_SSE"; ip++;
				papi.s_name[ip] = "FP_ARITH:256B_PACKED_DOUBLE";	//	4 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_AVX"; ip++;
				papi.s_name[ip] = "FP_ARITH:512B_PACKED_DOUBLE";	//	8 SIMD
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_AVXW"; ip++;
			} else {
				;	// no VECTOR support
				// hwpc_group.i_platform = 3;	// Haswell does not support VECTOR events
			}

		} else
		if (hwpc_group.platform == "SPARC64" ) {

			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
			//	[K and FX10]
			//		4 native events are supported for F.P.ops.
				hwpc_group.number[I_vector] += 4;
				papi.s_name[ip] = "FLOATING_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "FP_INS"; ip++;
				papi.s_name[ip] = "FMA_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "FMA_INS"; ip++;
				papi.s_name[ip] = "SIMD_FLOATING_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SIMD_FP"; ip++;
				papi.s_name[ip] = "SIMD_FMA_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SIMD_FMA"; ip++;
			}
			else if (hwpc_group.i_platform == 11 ) {
			//	[FX100]
			//		The following native events are supported for F.P.ops.
			//		It is not quite clear if they are precise for both double precision and single precision
				hwpc_group.number[I_vector] += 5;
				papi.s_name[ip] = "1FLOPS_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "1FP_INS"; ip++;
				papi.s_name[ip] = "2FLOPS_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "2FP_INS"; ip++;
				papi.s_name[ip] = "4FLOPS_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "4FP_INS"; ip++;
				papi.s_name[ip] = "8FLOPS_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "8FP_INS"; ip++;
				papi.s_name[ip] = "16FLOPS_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "16FP_INS"; ip++;
			/*
			The other combination might be
				FLOATING_INSTRUCTIONS
				FMA_INSTRUCTIONS
				SIMD_FLOATING_INSTRUCTIONS
				SIMD_FMA_INSTRUCTIONS
				4SIMD_FLOATING_INSTRUCTIONS
				4SIMD_FMA_INSTRUCTIONS
			Unfortunately, counting SIMD and 4SIMD instructions in one run causes an error.
			Also, it is quite confusing to map them into the actual f.p. instructions.
			So we dont use this combination.
			*/
			}
		} else

		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {

				hwpc_group.number[I_vector] += 4;
				papi.s_name[ip] = "FP_DP_SCALE_OPS_SPEC";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_SVE_op"; ip++;
				papi.s_name[ip] = "FP_DP_FIXED_OPS_SPEC";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "DP_FIX_op"; ip++;
				papi.s_name[ip] = "FP_SP_SCALE_OPS_SPEC";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_SVE_op"; ip++;
				papi.s_name[ip] = "FP_SP_FIXED_OPS_SPEC";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SP_FIX_op"; ip++;
		
			}
		}

	}


	else
// if (CACHE)

	if ( hwpc_group.env_str_hwpc == "CACHE" ) {
		hwpc_group.index[I_cache] = ip;
		hwpc_group.number[I_cache] = 0;

		if (hwpc_group.platform == "Xeon" ) {
			hwpc_group.number[I_cache] += 6;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:L1_HIT";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L1_HIT"; ip++;
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:HIT_LFB";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LFB_HIT"; ip++;
			papi.s_name[ip] = "L1_TCM"; papi.events[ip] = PAPI_L1_TCM; ip++;
			papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;
			//	"L2_RQSTS:DEMAND_DATA_RD_HIT" and "L2_RQSTS:PF_HIT" maybe more precise for cache hit/miss rate???
			//	We skip showing L3 here
			//	papi.s_name[ip] = "L3_TCM"; papi.events[ip] = PAPI_L3_TCM; ip++;
		} else

		if (hwpc_group.platform == "SPARC64" ) {
			// normal load and store counters can not be used together with cache counters
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				hwpc_group.number[I_cache] += 2;
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SIMD_LDST"; ip++;
			}
			else if (hwpc_group.i_platform == 11 ) {
				hwpc_group.number[I_cache] += 3;
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "2SIMD_LDST"; ip++;
				papi.s_name[ip] = "4SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "4SIMD_LDST"; ip++;
			}
			hwpc_group.number[I_cache] += 2;
			papi.s_name[ip] = "L1_TCM"; papi.events[ip] = PAPI_L1_TCM; ip++;
			papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;
		} else

		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
			hwpc_group.number[I_cache] += 5;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;	// == "LD_SPEC";
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;	// == "ST_SPEC";
			papi.s_name[ip] = "PAPI_L1_DCH";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L1_HIT"; ip++;
			papi.s_name[ip] = "L1_TCM"; papi.events[ip] = PAPI_L1_TCM; ip++;
			papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;
			}
		}
	}

	else
// if (CYCLE)
	if ( hwpc_group.env_str_hwpc == "CYCLE" ) {
		hwpc_group.index[I_cycle] = ip;
		hwpc_group.number[I_cycle] = 0;

		hwpc_group.number[I_cycle] += 2;
		papi.s_name[ip] = "TOT_CYC"; papi.events[ip] = PAPI_TOT_CYC; ip++;
		papi.s_name[ip] = "TOT_INS"; papi.events[ip] = PAPI_TOT_INS; ip++;

		if (hwpc_group.platform == "Xeon" ) {
			;
		} else
		if (hwpc_group.platform == "SPARC64" ) {
			;
		}
		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
				hwpc_group.number[I_cycle] += 2;
				papi.s_name[ip] = "PAPI_FP_INS";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "FP_inst"; ip++;
				papi.s_name[ip] = "PAPI_FMA_INS";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "FMA_inst"; ip++;
			}
		}
	}

	else
// if (LOADSTORE)
	if ( hwpc_group.env_str_hwpc == "LOADSTORE" ) {
		hwpc_group.index[I_loadstore] = ip;
		hwpc_group.number[I_loadstore] = 0;

		if (hwpc_group.platform == "Xeon" ) {
			hwpc_group.number[I_loadstore] = +2;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;

			if (hwpc_group.i_platform >= 2 && hwpc_group.i_platform <= 4 ) {
				// memory write operation via writeback and streaming-store for Sandybridge and Ivybridge
				hwpc_group.number[I_loadstore] += 2;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:WB:ANY_RESPONSE";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "WBACK_MEM"; ip++;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:STRM_ST:L3_MISS:SNP_ANY";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "STRMS_MEM"; ip++;
			} else {
				// The writeback and streaming-store events (WB and STRMS) are deleted on Skylake, somehow...
			}
		} else

		if (hwpc_group.platform == "SPARC64" ) {
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				hwpc_group.number[I_loadstore] += 2;
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SIMDLD+ST"; ip++;
			}
			else if (hwpc_group.i_platform == 11 ) {
				hwpc_group.number[I_loadstore] += 3;
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "2SIMD_LDST"; ip++;
				papi.s_name[ip] = "4SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "4SIMD_LDST"; ip++;
			}
		} else


		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
			hwpc_group.number[I_loadstore] += 8;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;
			papi.s_name[ip] = "ASE_SVE_LD_SPEC";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SVE_LOAD"; ip++;
			papi.s_name[ip] = "ASE_SVE_ST_SPEC";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SVE_STORE"; ip++;
			papi.s_name[ip] = "ASE_SVE_LD_MULTI_SPEC";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SVE_SMV_LD"; ip++;
			papi.s_name[ip] = "ASE_SVE_ST_MULTI_SPEC";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SVE_SMV_ST"; ip++;
			papi.s_name[ip] = "SVE_LD_GATHER_SPEC";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "GATHER_LD"; ip++;
			papi.s_name[ip] = "SVE_ST_SCATTER_SPEC";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SCATTER_ST"; ip++;
			}
		}
	}


// total number of traced events by PMlib
	papi.num_events = ip;

// end of hwpc_group selection

	#ifdef DEBUG_PRINT_PAPI
	//	if (my_rank == 0 && my_thread == 0) {
	if (my_rank == 0) {
		fprintf (stderr, " <createPapiCounterList> ends\n" );
		fprintf(stderr, " s_model_string=%s\n", s_model_string.c_str());

		fprintf(stderr, " platform: %s\n", hwpc_group.platform.c_str());
		fprintf(stderr, " HWPC_CHOOSER=%s\n", hwpc_group.env_str_hwpc.c_str());
		fprintf(stderr, " hwpc max output groups:%d\n", Max_hwpc_output_group);
		for (int i=0; i<Max_hwpc_output_group; i++) {
		fprintf(stderr, "  i:%d hwpc_group.number[i]=%d, hwpc_group.index[i]=%d\n",
						i, hwpc_group.number[i], hwpc_group.index[i]);
		}

		fprintf(stderr, "s_model_string=%s\n", s_model_string.c_str() );
		fprintf(stderr, "coreGHz=%f\n", hwpc_group.coreGHz);
		fprintf(stderr, "corePERF=%f\n", hwpc_group.corePERF);

		fprintf(stderr, " papi.num_events=%d, &papi=%p \n", papi.num_events, &papi.num_events);
		for (int i=0; i<papi.num_events; i++) {
		fprintf(stderr, "\t i=%d [%s] events[i]=%u, values[i]=%llu \n",
					i, papi.s_name[i].c_str(), papi.events[i], papi.values[i]);
		}
	}
	#endif
#endif // USE_PAPI
}


  /// Sort out the list of counters linked with the user input parameter
  ///
  /// @note this routine is called from both PerfMonitor and PerfWatch classes.
  /// each of the labeled measuring sections call this API

  /// PerfMonitor::gather() -> gather_and_stats() -> gatherHWPC() -> sortPapiCounterList()
  /// PerfMonitor::printThreads() -> ditto
  /// PerfMonitor::printProgress() -> ditto
  /// PerfMonitor::postTrace() -> ditto

  ///   PerfWatch::printDetailThreads() -> gatherHWPC() -> sortPapiCounterList()
  ///   PerfWatch::stop() -> sortPapiCounterList()	// special case when using OTF (#ifdef USE_OTF)
  ///

void PerfWatch::sortPapiCounterList (void)
{
#ifdef USE_PAPI

	int ip, jp, kp;
	double counts;
	double perf_rate=0.0;
	if ( m_time > 0.0 ) { perf_rate = 1.0/m_time; }

#ifdef DEBUG_PRINT_PAPI
//	#pragma omp barrier
//	#pragma omp critical
//	{
//	if (my_rank == 0) {
//		fprintf(stderr, "\t<sortPapiCounterList> [%15s], my_rank=%d, my_thread=%d, starting:\n",
//			m_label.c_str(), my_rank, my_thread);
//		for (int i = 0; i < 3; i++) {
//			fprintf(stderr, "\t\t i=%d [%8s] accumu[i]=%ld \n",
//			i, my_papi.s_sorted[i].c_str(), my_papi.accumu[i]);
//		}
//	}
//	}
#endif

// if (FLOPS)
	if ( hwpc_group.number[I_flops] > 0 ) {
		double d_flops, d_peak_normal, d_peak_ratio;
		counts=0.0;
		ip = hwpc_group.index[I_flops];
		jp=0;

		for(int i=0; i<hwpc_group.number[I_flops]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		my_papi.s_sorted[jp] = "Total_FP";
		my_papi.v_sorted[jp] = counts;
		jp++;

		d_flops = counts * perf_rate;		//	= counts / m_time ;
		my_papi.s_sorted[jp] = "[Flops] ";
		my_papi.v_sorted[jp] = d_flops;
		jp++;

		d_peak_ratio = d_flops / hwpc_group.corePERF;
		my_papi.s_sorted[jp] = "[%Peak] ";
		my_papi.v_sorted[jp] = d_peak_ratio * 100.0; 	//	percentage
		jp++;
	}

	else
// if (BANDWIDTH)
	if ( hwpc_group.number[I_bandwidth] > 0 ) {
		double d_load_ins, d_store_ins;
		double d_load_store, d_simd_load_store, d_xsimd_load_store;
		double d_hit_L2, d_miss_L2;
		double d_hit_LLC, d_miss_LLC;
		double d_Bytes_RD, d_Bytes_WR;
		double d_Bytes, bandwidth;
		double d_wb_L2;
		double cache_size;

		counts = 0.0;
		ip = hwpc_group.index[I_bandwidth];
		jp=0;
		for(int i=0; i<hwpc_group.number[I_bandwidth]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		ip = hwpc_group.index[I_bandwidth];

    	if (hwpc_group.platform == "Xeon" ) {
			if (hwpc_group.i_platform >= 2 && hwpc_group.i_platform <= 5 ) {
				cache_size = 64.0 ;		// that's Xeon

			d_hit_L2 = my_papi.accumu[ip] + my_papi.accumu[ip+1] ;	//	L2 data hit + L2 prefetch hit
			d_hit_LLC = my_papi.accumu[ip+2] ;	//	LLC data read hit + prefetch hit: "L3_HIT"
			d_miss_LLC = my_papi.accumu[ip+3] ;	//	LLC data read miss + prefetch miss: "L3_MISS"

			//	bandwidth = d_hit_L2 *64.0/m_time;	// L2 bandwidth
			bandwidth = d_hit_L2 * 64.0 * perf_rate;	// L2 bandwidth
			my_papi.s_sorted[jp] = "L2$ [B/s]" ;
			my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
			jp++;

			//	bandwidth = d_hit_LLC *64.0/m_time;	// LLC bandwidth
			bandwidth = d_hit_LLC * 64.0 * perf_rate;
			my_papi.s_sorted[jp] = "L3$ [B/s]" ;
			my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
			jp++;

			//	bandwidth = d_miss_LLC *64.0/m_time;	// Memory bandwidth
			bandwidth = d_miss_LLC * 64.0 * perf_rate;
			my_papi.s_sorted[jp] = "Mem [B/s]" ;
			my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
			jp++;

			// aggregated data bytes transferred out of L2 cache, L3 cache and memory
			d_Bytes = (d_hit_L2 + d_hit_LLC + d_miss_LLC) * 64.0;
			my_papi.s_sorted[jp] = "[Bytes]" ;
			my_papi.v_sorted[jp] = d_Bytes ;
			jp++;
			}

		} else
    	if (hwpc_group.platform == "SPARC64" ) {
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				cache_size = 128.0;		// K (and FX10) has 128 Byte $ line
			}
			else if (hwpc_group.i_platform == 11 ) {
				cache_size = 256.0;		// FX100 has 256 Byte $ line
			}
			// L2 hit  = L2_READ_DM + L2_READ_PF - (L2_MISS_DM + L2_MISS_PF)
			// L2 miss = L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF
			d_hit_L2  = my_papi.accumu[ip] + my_papi.accumu[ip+1] - (my_papi.accumu[ip+2] + my_papi.accumu[ip+3]) ;
			d_miss_L2 = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] + my_papi.accumu[ip+4] + my_papi.accumu[ip+5] ;

			// L2 hit BandWidth
			bandwidth = d_hit_L2 * cache_size * perf_rate;
			my_papi.s_sorted[jp] = "L2$ [B/s]" ;
			my_papi.v_sorted[jp] = bandwidth ;
			jp++;

			// Memory BandWidth
			bandwidth = d_miss_L2 * cache_size * perf_rate;
			my_papi.s_sorted[jp] = "Mem [B/s]" ;
			my_papi.v_sorted[jp] = bandwidth ; //* 1.0e-9;
			jp++;

			// aggregated data bytes transferred out of L2 cache and memory
			d_Bytes = (d_hit_L2 + d_miss_L2) * cache_size;
			my_papi.s_sorted[jp] = "[Bytes]" ;
			my_papi.v_sorted[jp] = d_Bytes ;
			jp++;

		} else
    	if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {

			// updated 2020/11/06
			// See the remarks in outputPapiCounterLegend()

				//	for(int jp=0; jp<hwpc_group.number[I_bandwidth]; jp++)
				//	{
				//		my_papi.v_sorted[jp] = my_papi.accumu[jp + hwpc_group.index[I_bandwidth]] ;
				//	}

				// Fugaku has 256 Byte $ line
				cache_size = 256.0;
				d_Bytes_RD = my_papi.accumu[ip+0] * cache_size ;
				d_Bytes_WR = my_papi.accumu[ip+1] * cache_size ;

				my_papi.s_sorted[jp] = "RD [Bytes]" ;
				my_papi.v_sorted[jp] = d_Bytes_RD ;
				jp++;

				my_papi.s_sorted[jp] = "WD [Bytes]" ;
				my_papi.v_sorted[jp] = d_Bytes_WR ;
				jp++;

				// Memory BandWidth using events BUS_READ_TOTAL_MEM + BUS_WRITE_TOTAL_MEM
				d_Bytes = d_Bytes_RD + d_Bytes_WR ;
				bandwidth = d_Bytes * perf_rate;
				my_papi.s_sorted[jp] = "Mem [B/s]" ;
				my_papi.v_sorted[jp] = bandwidth ; //* 1.0e-9;
				jp++;

				// Actual read + write bytes
				my_papi.s_sorted[jp] = "[Bytes]" ;
				my_papi.v_sorted[jp] = d_Bytes ;
				jp++;
			}
		}

	}

	else
// if (VECTOR)
	if ( hwpc_group.number[I_vector] > 0 ) {
		double fp_sp1, fp_sp2, fp_sp4, fp_sp8, fp_sp16;
		double fp_dp1, fp_dp2, fp_dp4, fp_dp8, fp_dp16;
		double fp_total, fp_vector;
		double vector_percent;
		double fp_spv, fp_dpv;

		vector_percent = 0.0;
		fp_vector = 0.0;
		fp_total = 1.0;
		counts = 0.0;
		ip = hwpc_group.index[I_vector];
		jp=0;
		for(int i=0; i<hwpc_group.number[I_vector]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		ip = hwpc_group.index[I_vector];

		if (hwpc_group.platform == "Xeon" ) {
			if (hwpc_group.i_platform == 2 ) {
				fp_sp1  = my_papi.accumu[ip] ;		//	FP_COMP_OPS_EXE:SSE_FP_SCALAR_SINGLE	//	SP_SINGLE
				fp_sp4  = my_papi.accumu[ip+1] ;	//	FP_COMP_OPS_EXE:SSE_PACKED_SINGLE	//	SP_SSE
				fp_sp8  = my_papi.accumu[ip+2] ;	//	SIMD_FP_256:PACKED_SINGLE			//	SP_AVX
				fp_dp1  = my_papi.accumu[ip+3] ;	//	FP_COMP_OPS_EXE:SSE_SCALAR_DOUBLE	//	DP_SINGLE
				fp_dp2  = my_papi.accumu[ip+4] ;	//	FP_COMP_OPS_EXE:SSE_FP_PACKED_DOUBLE	//	DP_SSE
				fp_dp4  = my_papi.accumu[ip+5] ;	//	SIMD_FP_256:PACKED_DOUBLE			//	DP_AVX
				fp_vector =          4.0*fp_sp4 + 8.0*fp_sp8 +          2.0*fp_dp2 + 4.0*fp_dp4;
				fp_total  = fp_sp1 + 4.0*fp_sp4 + 8.0*fp_sp8 + fp_dp1 + 2.0*fp_dp2 + 4.0*fp_dp4;
			} else
			if (hwpc_group.i_platform == 4 ) {
				fp_sp1  = my_papi.accumu[ip] ; 		//	 "FP_ARITH:SCALAR_SINGLE"; //	"SP_SINGLE";
				fp_sp4  = my_papi.accumu[ip+1] ; 	//	 "FP_ARITH:128B_PACKED_SINGLE"; //	"SP_SSE";
				fp_sp8  = my_papi.accumu[ip+2] ; 	//	 "FP_ARITH:256B_PACKED_SINGLE"; //	"SP_AVX";
				fp_dp1  = my_papi.accumu[ip+3] ; 	//	 "FP_ARITH:SCALAR_DOUBLE"; //	"DP_SINGLE";
				fp_dp2  = my_papi.accumu[ip+4] ; 	//	 "FP_ARITH:128B_PACKED_DOUBLE"; //	"DP_SSE";
				fp_dp4  = my_papi.accumu[ip+5] ; 	//	 "FP_ARITH:256B_PACKED_DOUBLE"; //	"DP_AVX";
				// DPP and FMA events have been counted twice by PAPI
				fp_vector = 4.0*fp_sp4 + 8.0*fp_sp8 + 2.0*fp_dp2 + 4.0*fp_dp4;
				fp_total  = fp_sp1 + fp_dp1 + fp_vector;
			} else
			if (hwpc_group.i_platform == 5 ) {
				fp_sp1  = my_papi.accumu[ip] ; 		//	 "FP_ARITH:SCALAR_SINGLE"; //	"SP_SINGLE";
				fp_sp4  = my_papi.accumu[ip+1] ; 	//	 "FP_ARITH:128B_PACKED_SINGLE"; //	"SP_SSE";
				fp_sp8  = my_papi.accumu[ip+2] ; 	//	 "FP_ARITH:256B_PACKED_SINGLE"; //	"SP_AVX";
				fp_sp16 = my_papi.accumu[ip+3] ; 	//	 "FP_ARITH:512B_PACKED_SINGLE"; //	"SP_AVXW";
				fp_dp1  = my_papi.accumu[ip+4] ; 	//	 "FP_ARITH:SCALAR_DOUBLE"; //	"DP_SINGLE";
				fp_dp2  = my_papi.accumu[ip+5] ; 	//	 "FP_ARITH:128B_PACKED_DOUBLE"; //	"DP_SSE";
				fp_dp4  = my_papi.accumu[ip+6] ; 	//	 "FP_ARITH:256B_PACKED_DOUBLE"; //	"DP_AVX";
				fp_dp8  = my_papi.accumu[ip+7] ; 	//	 "FP_ARITH:512B_PACKED_DOUBLE"; //	"DP_AVXW";
				fp_vector = 4.0*fp_sp4 + 8.0*fp_sp8 + 16.0*fp_sp16 + 2.0*fp_dp2 + 4.0*fp_dp4 + 8.0*fp_dp8;
				fp_total  = fp_sp1 + fp_dp1 + fp_vector;
			}
			if (m_exclusive) {
				if ( fp_total > 0.0 ) {
				vector_percent = fp_vector/fp_total;
				}
			}

		} else
    	if (hwpc_group.platform == "SPARC64" ) {
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
			//	[K and FX10]
				fp_dp1  = my_papi.accumu[ip] ;		//	FLOATING_INSTRUCTIONS
				fp_dp2  = my_papi.accumu[ip+1] ;	//	FMA_INSTRUCTIONS
				fp_dp2 += my_papi.accumu[ip+2] ;	//	SIMD_FLOATING_INSTRUCTIONS
				fp_dp4  = my_papi.accumu[ip+3] ;	//	SIMD_FMA_INSTRUCTIONS
				fp_vector = (         2.0*fp_dp2 + 4.0*fp_dp4);
				fp_total  = (fp_dp1 + 2.0*fp_dp2 + 4.0*fp_dp4);
			} else
			if (hwpc_group.i_platform == 11 ) {
			//	[FX100]
				//	Rough approximation is done baed on XSIMD event count.
				//	See comments in the API createPapiCounterList above.
				fp_dp1  = my_papi.accumu[ip] ;
				fp_dp2  = my_papi.accumu[ip+1] ;
				fp_dp4  = my_papi.accumu[ip+2] ;
				fp_dp8  = my_papi.accumu[ip+3] ;
				fp_dp16  = my_papi.accumu[ip+3] ;
				fp_vector = (                      4.0*fp_dp4 + 8.0*fp_dp8 + 16.0*fp_dp16);
				fp_total  = (fp_dp1 + 2.0*fp_dp2 + 4.0*fp_dp4 + 8.0*fp_dp8 + 16.0*fp_dp16);
			}
			if (m_exclusive) {
				if ( fp_total > 0.0 ) {
				vector_percent = fp_vector/fp_total;
				}
			}

		} else

    	if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {

			//	total OPS =  vector OPS + scalar OPS = PAPI_FP_OPS
			//		vector OPS = operations by SVE instructions = vector DP ops + vector SP ops
			//		scalar OPS = operations by scalar and armv8simd instructions = scalar DP ops + scalar SP ops
			//
			//	vector OPS = 4*FP_SCALE_OPS_SPEC = 4*FP_DP_SCALE_OPS_SPEC + 4*FP_SP_SCALE_OPS_SPEC
			//	scalar OPS = FP_FIXED_OPS_SPEC = FP_DP_FIXED_OPS_SPEC + FP_SP_FIXED_OPS_SPEC
			//	The number "4" in the above formula has the special meaning in arm SVE context.
			//
			//	The OPS can also be grouped into FMA and non-FMA ops.
			//		vector OPS = FMA vector OPS + non-FMA vector OPS
			//		scalar OPS = FMA scalar OPS + non-FMA scalar OPS
			//	The total number of FMA instructions is available, but the ratio of FMA v.s. non-FMA is not available.
			//	So roughly averaged approximation is applied for both DP and SP
			//		(FMA vector OPS)/(vector OPS) = (FMA scalar OPS)/(scalar OPS) = RF
			//		x1 : #of_inst_vector_FMA_DP	//	vector FMA f.p. ops in DP = 16*x1
			//		y1 : #of_inst_vector_FMA_SP	//	vector FMA f.p. ops in SP = 32*y1
			//		x2 : #of_inst_scalar_FMA_DP	//	scalar FMA f.p. ops in DP = 2*x2
			//		y2 : #of_inst_scalar_FMA_SP	//	scalar FMA f.p. ops in SP = 2*y2

				//	vDP : fops_vector_DP = vector Double Precision floating point operations
				//	vSP : fops_vector_SP = vector Single Precision floating point operations
				//	sDP : fops_scalar_DP = scalar Double Precision floating point operations
				//	sSP : fops_scalar_SP = scalar Single Precision floating point operations

				// vDP = 4*FP_DP_SCALE_OPS_SPEC				// formula-(1)	ops.
				// vSP = 4*FP_SP_SCALE_OPS_SPEC				// formula-(2)	ops.
				// sDP = FP_DP_FIXED_OPS_SPEC				// formula-(3)	ops.
				// sSP = FP_SP_FIXED_OPS_SPEC				// formula-(4)	ops.
				// FMA ops = 16*x1 + 32*y1 + 2*x2 + 2*y2			// formula-(5)	FMA ops.
				// FMA instructions = FMA_INS = x1 + y1 + x2 + y2	// formula-(6)	FMA inst.
				// 16*x1/vDP = 32*y1/vSP = 2*x2/sDP = 2*y2/sSP = RF	// formula-(7)	// approximation
					// x1 = RF*vDP/16
					// y1 = RF*vSP/32
					// x2 = RF*sDP/2
					// y2 = RF*sSP/2
				// put into formula-(6)
				// FMA_INS = RF*(vDP/16 + vSP/32 + sDP/2 + sSP/2)
				// RF = FMA_INS / (vDP/16 + vSP/32 + sDP/2 + sSP/2)	// formula-(12)
				// put into formula-(5)
				// FMA ops = RF*(vDP+vSP+sDP+sSP) == RF*PAPI_FP_OPS


			// SVE f.p. operations are counted uniquely, and needs to scale 4x (512/128) to obtain actual ops
			//	i.e. _SCALE_OPS_ values should be multiplied by 4
				fp_dpv  = 4 * my_papi.accumu[ip] ;
				fp_dp1  = my_papi.accumu[ip+1] ;
				fp_spv  = 4 * my_papi.accumu[ip+2] ;
				fp_sp1  = my_papi.accumu[ip+3] ;
				fp_vector =                   fp_dpv + fp_spv ;
				fp_total  = fp_dp1 + fp_sp1 + fp_dpv + fp_spv ;

			// correction of v_sorted values
				my_papi.v_sorted[ip] = fp_dpv;
				my_papi.v_sorted[ip+2] = fp_spv;

			}
			if (m_exclusive) {
				if ( fp_total > 0.0 ) {
				vector_percent = fp_vector/fp_total;
				}
			}
		}

		my_papi.s_sorted[jp] = "Total_FP" ;
		my_papi.v_sorted[jp] = fp_total;
		jp++;
		my_papi.s_sorted[jp] = "Vector_FP" ;
		my_papi.v_sorted[jp] = fp_vector;
		jp++;
		my_papi.s_sorted[jp] = "[Vector %]" ;
		my_papi.v_sorted[jp] = vector_percent * 100.0;
		jp++;

	}

	else
// if (CACHE)
	if ( hwpc_group.number[I_cache] > 0 ) {
		double d_load_ins, d_store_ins;
		double d_load_store, d_simd_load_store, d_xsimd_load_store;
		double d_hit_LFB, d_hit_L1, d_miss_L1, d_miss_L2, d_miss_L3;
		double d_L1_ratio, d_L2_ratio, d_cache_transaction;

		ip = hwpc_group.index[I_cache];
		jp=0;
		for(int i=0; i<hwpc_group.number[I_cache]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		ip = hwpc_group.index[I_cache];

		if (hwpc_group.platform == "Xeon" ) {
			d_load_ins  = my_papi.accumu[ip] ;	//	PAPI_LD_INS
			d_store_ins = my_papi.accumu[ip+1] ;	//	PAPI_SR_INS
			d_hit_L1  = my_papi.accumu[ip+2] ;	//	MEM_LOAD_UOPS_RETIRED:L1_HIT
			d_hit_LFB = my_papi.accumu[ip+3] ;	//	MEM_LOAD_UOPS_RETIRED:HIT_LFB
			d_miss_L1 = my_papi.accumu[ip+4] ;	//	PAPI_L1_TCM
			d_miss_L2 = my_papi.accumu[ip+5] ;	//	PAPI_L2_TCM
			//	d_miss_L3 = my_papi.accumu[ip+6] ;	//	PAPI_L3_TCM // we skip showing L3 here

			d_cache_transaction = d_hit_L1 + d_hit_LFB + d_miss_L1 ;
			if ( d_cache_transaction > 0.0 ) {
				//	d_L1_ratio = (d_hit_LFB + d_hit_L1) / (d_hit_LFB + d_hit_L1 + d_miss_L1) ;
				//	d_L2_ratio = (d_miss_L1 - d_miss_L2) / (d_hit_LFB + d_hit_L1 + d_miss_L1) ;
				d_L1_ratio = (d_hit_LFB + d_hit_L1) / d_cache_transaction;
				d_L2_ratio = (d_miss_L1 - d_miss_L2) / d_cache_transaction;
			} else {
				d_L1_ratio = d_L2_ratio = 0.0;
			}
			my_papi.s_sorted[jp] = "[L1$ hit%]";
			my_papi.v_sorted[jp] = d_L1_ratio * 100.0;
			jp++;
			my_papi.s_sorted[jp] = "[L2$ hit%]";
			my_papi.v_sorted[jp] = d_L2_ratio * 100.0;
			jp++;

			#ifdef DEBUG_PRINT_PAPI
			#pragma omp barrier
			#pragma omp critical
			if (my_rank == 0) {
			fprintf(stderr, "debug <sortPapiCounterList> d_load_ins=%e, d_store_ins=%e, d_hit_L1=%e, d_hit_LFB=%e, d_miss_L1=%e, d_miss_L2=%e, d_cache_transaction=%e, d_L1_ratio=%e, d_L2_ratio=%e\n",
			d_load_ins, d_store_ins, d_hit_L1, d_hit_LFB, d_miss_L1, d_miss_L2, d_cache_transaction, d_L1_ratio, d_L2_ratio);
			}
			#endif
		} else

		if (hwpc_group.platform == "SPARC64" ) {
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
			//	[K and FX10] :  Load+Store	+ 2SIMD Load+Store
				d_load_store = my_papi.accumu[ip] + my_papi.accumu[ip+1] ;
				kp=2;
			} else
			if (hwpc_group.i_platform == 11 ) {
			//	[FX100]	:  Load+Store	+ 2SIMD Load+Store	+ 4SIMD Load+Store
				d_load_store = my_papi.accumu[ip] + my_papi.accumu[ip+1] + my_papi.accumu[ip+2] ;
				kp=3;
			}

			d_miss_L1 = my_papi.accumu[ip +kp ] ;	//	PAPI_L1_TCM
			d_miss_L2 = my_papi.accumu[ip +kp +1 ] ;	//	PAPI_L2_TCM

			if ( d_load_store > 0.0 ) {
				d_L1_ratio = (d_load_store - d_miss_L1) / d_load_store;
				d_L2_ratio = (d_miss_L1 - d_miss_L2) / d_load_store;
			} else {
				d_L1_ratio = d_L2_ratio = 0.0;
			}
			my_papi.s_sorted[jp] = "[L1$ hit%]";
			my_papi.v_sorted[jp] = d_L1_ratio * 100.0;
			jp++;
			my_papi.s_sorted[jp] = "[L2$ hit%]";
			my_papi.v_sorted[jp] = d_L2_ratio * 100.0;
			jp++;
		} else

		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
			d_load_ins  = my_papi.accumu[ip] ;	//	PAPI_LD_INS
			d_store_ins = my_papi.accumu[ip+1] ;	//	PAPI_SR_INS
			d_hit_L1  = my_papi.accumu[ip+2] ;	//	PAPI_L1_DCH
			d_miss_L1 = my_papi.accumu[ip+3] ;	//	PAPI_L1_TCM
			d_miss_L2 = my_papi.accumu[ip+4] ;	//	PAPI_L2_TCM
			d_load_store = d_load_ins + d_store_ins;
			if ( d_load_store > 0.0 ) {
				d_L1_ratio = (d_load_store - d_miss_L1) / d_load_store;
				d_L2_ratio = (d_miss_L1 - d_miss_L2) / d_load_store;
			} else {
				d_L1_ratio = d_L2_ratio = 0.0;
			}
			my_papi.s_sorted[jp] = "[L1$ hit%]";
			my_papi.v_sorted[jp] = d_L1_ratio * 100.0;
			jp++;
			my_papi.s_sorted[jp] = "[L2$ hit%]";
			my_papi.v_sorted[jp] = d_L2_ratio * 100.0;
			jp++;
			}
			#ifdef DEBUG_PRINT_PAPI
			#pragma omp barrier
			#pragma omp critical
			if (my_rank == 0) {
			fprintf(stderr, "debug <sortPapiCounterList> d_load_ins=%e, d_store_ins=%e, d_hit_L1=%e, d_miss_L1=%e, d_miss_L2=%e, d_L1_ratio=%e, d_L2_ratio=%e\n",
			d_load_ins, d_store_ins, d_hit_L1, d_miss_L1, d_miss_L2, d_L1_ratio, d_L2_ratio);
			}
			#endif
		}

		my_papi.s_sorted[jp] = "[L*$ hit%]";
		my_papi.v_sorted[jp] = (d_L1_ratio + d_L2_ratio) * 100.0;
		jp++;

	}

	else
// if (CYCLE)
	if ( hwpc_group.number[I_cycle] > 0 ) {
		double d_fp_ins, d_fma_ins, fma_percent;

		ip = hwpc_group.index[I_cycle];
		jp=0;
		for(int i=0; i<hwpc_group.number[I_cycle] ; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		ip = hwpc_group.index[I_cycle];

		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
			//	Ratio of FMA instructions over total f.p. instructions = d_fma_ins / d_fp_ins
				d_fp_ins  = my_papi.accumu[ip+2] ;
				d_fma_ins = my_papi.accumu[ip+3] ;
				fma_percent = 0.0;
				if ( d_fp_ins > 0.0 ) {
					fma_percent = 100.0* d_fma_ins / d_fp_ins;
				} else {
					fma_percent = 0.0;
				}
				my_papi.s_sorted[jp] = "[FMA_ins%]" ;
				my_papi.v_sorted[jp] = fma_percent;
				jp++;
			}
		}

		//	events[0] = PAPI_TOT_CYC;
		//	events[1] = PAPI_TOT_INS;
		my_papi.s_sorted[jp] = "[Ins/cyc]" ;
		if ( my_papi.v_sorted[ip] > 0.0 ) {
			my_papi.v_sorted[jp] = my_papi.v_sorted[ip+1] / my_papi.v_sorted[ip];
		} else {
			my_papi.v_sorted[jp] = 0.0;
		}
		jp++;
	}

	else
// if (LOADSTORE)

	if ( hwpc_group.number[I_loadstore] > 0 ) {
		double d_load_ins, d_store_ins, d_load_store_ins, d_simd_load_store_ins;
		double d_writeback_MEM, d_streaming_MEM;
		double bandwidth;
		double d_sve_load_ins, d_sve_store_ins;
		double vector_percent;
		counts = 0.0;
		ip = hwpc_group.index[I_loadstore];
		jp=0;
		for(int i=0; i<hwpc_group.number[I_loadstore]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		ip = hwpc_group.index[I_loadstore];

    	if (hwpc_group.platform == "Xeon" ) {
			if (hwpc_group.i_platform >= 2 && hwpc_group.i_platform <= 4 ) {
				// memory write operation via writeback and streaming-store for Sandybridge and Ivybridge
				// this event is not counted on Skylake somehow...
				ip = hwpc_group.index[I_loadstore];
				d_writeback_MEM = my_papi.accumu[ip+2] ;	//	OFFCORE_RESPONSE_0:WB:ANY_RESPONSE
				d_streaming_MEM = my_papi.accumu[ip+3] ;	//	OFFCORE_RESPONSE_0:STRM_ST:L3_MISS:SNP_ANY
				bandwidth = (d_writeback_MEM + d_streaming_MEM) * 64.0 * perf_rate;
				// We don't report this bandwidth by writeback and streaming store
					//	my_papi.s_sorted[jp] = "wb+strm.st" ;
					//	my_papi.v_sorted[jp] = bandwidth ;
					//	jp++;
			}
			vector_percent = 0.0;	// vectorized load and store instructions are not defined on Xeon
		} else

		if (hwpc_group.platform == "SPARC64" ) {
			// on K/FX10/FX100, the load/store events do NOT include SIMD load/store events
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				d_load_store_ins      = my_papi.accumu[ip] ;
				d_simd_load_store_ins = my_papi.accumu[ip+1] ;
				vector_percent = d_simd_load_store_ins/(d_load_store_ins+d_simd_load_store_ins);
			}
			else if (hwpc_group.i_platform == 11 ) {
				d_load_store_ins      = my_papi.accumu[ip] ;
				d_simd_load_store_ins = my_papi.accumu[ip+1] + my_papi.accumu[ip+2] ;
				vector_percent = d_simd_load_store_ins/(d_load_store_ins+d_simd_load_store_ins);
			}
		} else

		if (hwpc_group.platform == "A64FX" ) {
			// On Fugaku, the load/store events include SIMD load/store events
			if (hwpc_group.i_platform == 21 ) {
				d_load_ins  = my_papi.accumu[ip] ;		//	PAPI_LD_INS	counts both armv8 and SVE load
				d_store_ins = my_papi.accumu[ip+1] ;	//	PAPI_SR_INS	counts both armv8 and SVE store
				d_sve_load_ins = my_papi.accumu[ip+2] + my_papi.accumu[ip+4] + my_papi.accumu[ip+6];	// SVE load
				d_sve_store_ins = my_papi.accumu[ip+3] + my_papi.accumu[ip+5] + my_papi.accumu[ip+7];	// SVE store
				vector_percent = (d_sve_load_ins+d_sve_store_ins)/(d_load_ins+d_store_ins) ;
			}
		}
		my_papi.s_sorted[jp] = "[Vector %]" ;
		my_papi.v_sorted[jp] = vector_percent * 100.0;
		jp++;
	}
    	
// count the number of reported events and derived matrices
	my_papi.num_sorted = jp;

#ifdef DEBUG_PRINT_PAPI_THREADS
	#pragma omp barrier
	#pragma omp critical
	{
	if (my_rank == 0) {
		fprintf(stderr, "\t<sortPapiCounterList> [%15s], my_rank=%d, my_thread=%d, returning m_time=%e\n",
			m_label.c_str(), my_rank, my_thread, m_time );
		for (int i = 0; i < my_papi.num_sorted; i++) {
			fprintf(stderr, "\t\t i=%d [%8s] v_sorted[i]=%e \n",
			i, my_papi.s_sorted[i].c_str(), my_papi.v_sorted[i]);
		}
	}
	}
#endif

#endif // USE_PAPI
}



  /// print the HWPC report Section Label string line, and the header line with event names
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] s_label ラベル
  ///
void PerfWatch::outputPapiCounterHeader (FILE* fp, std::string s_label)
{
#ifdef USE_PAPI

	fprintf(fp, "Section Label : %s%s\n", s_label.c_str(), m_exclusive ? "" : "(*)" );

	std::string s;
	int ip, jp, kp;
	//	fprintf (fp, "Header  ID :");
	fprintf (fp, "MPI rankID :");
	for(int i=0; i<my_papi.num_sorted; i++) {
		kp = my_papi.s_sorted[i].find_last_of(':');
		if ( kp < 0) {
			s = my_papi.s_sorted[i];
		} else {
			s = my_papi.s_sorted[i].substr(kp+1);
		}
		fprintf (fp, " %10.10s", s.c_str() );
	} fprintf (fp, "\n");

#endif // USE_PAPI
}



  /// Display the HWPC measured values and their derived values
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///
void PerfWatch::outputPapiCounterList (FILE* fp)
{
#ifdef USE_PAPI
	int iret;
	if (my_rank == 0) {
	// print the HWPC event values and their derived values
	for (int i=0; i<num_process; i++) {
		fprintf(fp, "Rank %5d :", i);
		for(int n=0; n<my_papi.num_sorted; n++) {
			fprintf (fp, "  %9.3e", fabs(m_sortedArrayHWPC[i*my_papi.num_sorted + n]));
		}
		fprintf (fp, "\n");
	}
	}

#endif // USE_PAPI
}


  ///   Groupに含まれるMPIプロセスのHWPC測定結果を区間毎に出力
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] p_group プロセスグループ番号。
  ///   @param[in] pp_ranks int*型 groupを構成するrank番号配列へのポインタ
  ///
void PerfWatch::outputPapiCounterGroup (FILE* fp, MPI_Group p_group, int* pp_ranks)
{
#ifdef USE_PAPI
	int iret, g_np, ip;
	iret =
	MPI_Group_size(p_group, &g_np);
	if (iret != MPI_SUCCESS) {
			fprintf (fp, "  *** <outputPapiCounterGroup> MPI_Group_size failed. %x \n", p_group);
			//	PM_Exit(0);
			return;
	}
	if (my_rank == 0) {
	for (int i=0; i<g_np; i++) {
		ip = pp_ranks[i];
		fprintf(fp, "Rank %5d :", ip);
		for(int n=0; n<my_papi.num_sorted; n++) {
			fprintf (fp, "  %9.3e", fabs(m_sortedArrayHWPC[ip*my_papi.num_sorted + n]));
		}
		fprintf (fp, "\n");
	}
	}
#endif // USE_PAPI
}


  /// Display the HWPC legend
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///
void PerfWatch::outputPapiCounterLegend (FILE* fp)
{
#ifdef USE_PAPI

// TODO: We should simplify these too many fprintf() calls into one fprintf() call. sometime...

	const PAPI_hw_info_t *hwinfo = NULL;
	std::string s_model_string;
	std::string s_vendor_string;
	using namespace std;

	fprintf(fp, "\n\t Symbols in hardware performance counter (HWPC) report:\n\n" );
	hwinfo = PAPI_get_hardware_info();
	if (hwinfo == NULL) {
		//	fprintf (fp, "\n\t<PAPI_get_hardware_info> failed. \n" );
		fprintf(fp, "\n\t HWPC was not initialized, so automatic CPU detection and HWPC legend was disabled.\n");
		fprintf(fp, "\t In order to enable HWPC feature, HWPC_CHOOSER env. var. must be set for the job as:\n");
		fprintf(fp, "\t $ export HWPC_CHOOSER=FLOPS # [FLOPS|BANDWIDTH|VECTOR|CACHE|CYCLE|LOADSTORE]\n\n");
		return;
	}

    if (s_model_string.empty() && s_vendor_string.find( "ARM" ) != string::npos ) {
		identifyARMplatform ();
	}
    if ( hwpc_group.i_platform == 21 ) {
		s_model_string = "Fugaku A64FX" ;
	} else {
		s_model_string = hwinfo->model_string;
	}
	fprintf(fp, "\t Detected CPU architecture: %s \n", s_model_string.c_str());

	fprintf(fp, "\t The available HWPC_CHOOSER values and their HWPC events for this CPU are shown below.\n");
	fprintf(fp, "\n");

// FLOPS
	fprintf(fp, "\t HWPC_CHOOSER=FLOPS:\n");
	if (hwpc_group.platform == "Xeon" ) {
		if (hwpc_group.i_platform != 3 ) {
	fprintf(fp, "\t\t SP_OPS:    single precision floating point operations\n");
	fprintf(fp, "\t\t DP_OPS:    double precision floating point operations\n");
		} else {
	fprintf(fp, "\t\t * Haswell processor does not have floating point operation counters,\n");
	fprintf(fp, "\t\t * so PMlib does not produce HWPC report for FLOPS and VECTOR groups.\n");
		}
	} else

	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\t FP_OPS:    floating point operations\n");
	} else

	if (hwpc_group.platform == "A64FX" ) {
	fprintf(fp, "\t\t SP_OPS:    single precision floating point operations\n");
	fprintf(fp, "\t\t DP_OPS:    double precision floating point operations\n");
	}
	fprintf(fp, "\t\t Total_FP:  total floating point operations\n");
	fprintf(fp, "\t\t [Flops]:   floating point operations per second \n");
	fprintf(fp, "\t\t [%%Peak]:   sustained performance over peak performance\n");

// BANDWIDTH
	fprintf(fp, "\t HWPC_CHOOSER=BANDWIDTH:\n");
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\t LOAD_INS:  memory load instructions\n");
	fprintf(fp, "\t\t STORE_INS: memory store instructions\n");
	fprintf(fp, "\t\t L2_RD_HIT: L2 cache data read hit \n");
	fprintf(fp, "\t\t L2_PF_HIT: L2 cache data prefetch hit \n");
	fprintf(fp, "\t\t L3_HIT:    Last Level Cache data read hit \n");
	fprintf(fp, "\t\t L3_MISS:   Last Level Cache data read miss \n");
	fprintf(fp, "\t\t L2$ [B/s]: L2 cache working bandwidth responding to demand read and prefetch\n");
	fprintf(fp, "\t\t L3$ [B/s]: Last Level Cache bandwidth responding to demand read and prefetch\n");
	fprintf(fp, "\t\t Mem [B/s]: Memory bandwidth responding to demand read and prefetch\n");
	fprintf(fp, "\t\t [Bytes]  : aggregated data bytes transferred out of L2, L3 cache and memory\n");
	} else

	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\t LD+ST:      memory load/store instructions\n");
	fprintf(fp, "\t\t XSIMDLD+ST: memory load/store extended SIMD instructions\n");
	fprintf(fp, "\t\t L2_MISS_DM: L2 cache misses by demand request\n");
	fprintf(fp, "\t\t L2_MISS_PF: L2 cache misses by prefetch request\n");
	fprintf(fp, "\t\t L2_WB_DM:   writeback by demand L2 cache misses \n");
	fprintf(fp, "\t\t L2_WB_PF:   writeback by prefetch L2 cache misses \n");
	fprintf(fp, "\t\t L2$ [B/s]:  L2 cache bandwidth responding to demand read and prefetch \n");
	fprintf(fp, "\t\t Mem [B/s]:  Memory bandwidth responding to demand read, prefetch and writeback reaching memory\n");
	fprintf(fp, "\t\t [Bytes]  :  aggregated data bytes transferred out of L2 cache and memory\n");
	} else

	if (hwpc_group.platform == "A64FX" ) {
	fprintf(fp, "\t\t CMG_bus_RD:  CMG local memory read counts\n");
	fprintf(fp, "\t\t CMG_bus_WR:  CMG local memory write counts\n");
	fprintf(fp, "\t\t RD [Bytes]:  CMG local memory read bytes\n");
	fprintf(fp, "\t\t CMG_bus_WR:  CMG local memory write bytes\n");
	fprintf(fp, "\t\t Mem [B/s]:  CMG local memory read&write bandwidth \n");
	fprintf(fp, "\t\t [Bytes]  :  CMG local memory read&write bytes\n");
	}

// VECTOR
	fprintf(fp, "\t HWPC_CHOOSER=VECTOR:\n");
	if (hwpc_group.platform == "Xeon" ) {
		if (hwpc_group.i_platform != 3 ) {
	fprintf(fp, "\t\t SP_SINGLE:  single precision f.p. scalar instructions\n");
	fprintf(fp, "\t\t SP_SSE:     single precision f.p. SSE instructions\n");
	fprintf(fp, "\t\t SP_AVX:     single precision f.p. 256-bit AVX instructions\n");
	fprintf(fp, "\t\t SP_AVXW:    single precision f.p. 512-bit AVX instructions\n");
	fprintf(fp, "\t\t DP_SINGLE:  double precision f.p. scalar instructions\n");
	fprintf(fp, "\t\t DP_SSE:     double precision f.p. SSE instructions\n");
	fprintf(fp, "\t\t DP_AVX:     double precision f.p. 256-bit AVX instructions\n");
	fprintf(fp, "\t\t DP_AVXW:    double precision f.p. 512-bit AVX instructions\n");
	fprintf(fp, "\t\t Total_FP:   total floating point operations \n");
	fprintf(fp, "\t\t Vector_FP:  floating point operations by vector instructions\n");
	fprintf(fp, "\t\t [Vector %%]: percentage of vectorized f.p. operations\n");
		} else {
	fprintf(fp, "\t\t Haswell processor does not have floating point operation counters,\n");
	fprintf(fp, "\t\t so PMlib does not produce full HWPC report for FLOPS and VECTOR groups.\n");
		}
	} else

	if (hwpc_group.platform == "SPARC64" ) {
		if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
	fprintf(fp, "\t\t FP_INS:     f.p. instructions\n");
	fprintf(fp, "\t\t FMA_INS:    FMA instructions\n");
	fprintf(fp, "\t\t SIMD_FP:    SIMD f.p. instructions\n");
	fprintf(fp, "\t\t SIMD_FMA:   SIMD FMA instructions\n");
		}
		if (hwpc_group.i_platform == 11 ) {
	fprintf(fp, "\t\t 1FP_INS:    1 f.p. op instructions\n");
	fprintf(fp, "\t\t 2FP_INS:    2 f.p. ops instructions\n");
	fprintf(fp, "\t\t 4FP_INS:    4 f.p. ops instructions\n");
	fprintf(fp, "\t\t 8FP_INS:    8 f.p. ops instructions\n");
	fprintf(fp, "\t\t 16FP_INS : 16 f.p. ops instructions\n");
		}
	fprintf(fp, "\t\t Total_FP:   total floating point operations \n");
	fprintf(fp, "\t\t Vector_FP:  floating point operations by vector instructions\n");
	fprintf(fp, "\t\t [Vector %%]: percentage of vectorized f.p. operations\n");
	} else

	if (hwpc_group.platform == "A64FX" ) {
	fprintf(fp, "\t\t DP_SVE_op:  double precision f.p. ops by SVE instructions\n");
	fprintf(fp, "\t\t DP_FIX_op:  double precision f.p. ops by scalar/armv8 instructions\n");
	fprintf(fp, "\t\t SP_SVE_op:  single precision f.p. ops by SVE instructions\n");
	fprintf(fp, "\t\t SP_FIX_op:  single precision f.p. ops by scalar/armv8 instructions\n");
	fprintf(fp, "\t\t Total_FP:   total floating point operations \n");
	fprintf(fp, "\t\t Vector_FP:  floating point operations by vector instructions\n");
	fprintf(fp, "\t\t [Vector %%]: percentage of vectorized f.p. operations\n");
	}

// CACHE
	fprintf(fp, "\t HWPC_CHOOSER=CACHE:\n");
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\t LOAD_INS:   memory load instructions\n");
	fprintf(fp, "\t\t STORE_INS:  memory store instructions\n");
	fprintf(fp, "\t\t L1_HIT:     L1 data cache hits\n");
	fprintf(fp, "\t\t LFB_HIT:    Cache Line Fill Buffer hits\n");
	fprintf(fp, "\t\t L1_TCM:     L1 data cache activities leading to line replacements\n");
	fprintf(fp, "\t\t L2_TCM:     L2 cache demand access misses\n");
	//	fprintf(fp, "\t\t L3_TCM: level 3 total cache misses by demand\n");
	fprintf(fp, "\t\t [L1$ hit%%]: data access hit(%%) in L1 data cache and Line Fill Buffer\n");
	fprintf(fp, "\t\t [L2$ hit%%]: data access hit(%%) in L2 cache\n");
	fprintf(fp, "\t\t [L*$ hit%%]: sum of hit(%%) in L1 and L2 cache\n");
	} else

	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\t LD+ST:      memory load/store instructions\n");
	fprintf(fp, "\t\t 2SIMD:LDST: memory load/store SIMD instructions(2SIMD)\n");
	fprintf(fp, "\t\t 4SIMD:LDST: memory load/store extended SIMD instructions(4SIMD)\n");
	fprintf(fp, "\t\t L1_TCM:     L1 cache misses (by demand and by prefetch)\n");
	fprintf(fp, "\t\t L2_TCM:     L2 cache misses (by demand and by prefetch)\n");
	fprintf(fp, "\t\t [L1$ hit%%]: data access hit(%%) in L1 cache \n");
	fprintf(fp, "\t\t [L2$ hit%%]: data access hit(%%) in L2 cache\n");
	fprintf(fp, "\t\t [L*$ hit%%]: sum of hit(%%) in L1 and L2 cache\n");
	} else

	if (hwpc_group.platform == "A64FX" ) {
	fprintf(fp, "\t\t LOAD_INS:   memory load instructions\n");
	fprintf(fp, "\t\t STORE_INS:  memory store instructions\n");
	fprintf(fp, "\t\t L1_HIT:     L1 data cache hits\n");
	fprintf(fp, "\t\t L1_TCM:     L1 data cache misses\n");
	fprintf(fp, "\t\t L2_TCM:     L2 cache misses\n");
	fprintf(fp, "\t\t [L1$ hit%%]: data access hit(%%) in L1 cache \n");
	fprintf(fp, "\t\t [L2$ hit%%]: data access hit(%%) in L2 cache\n");
	fprintf(fp, "\t\t [L*$ hit%%]: sum of hit(%%) in L1 and L2 cache\n");
	}


// LOADSTORE
	fprintf(fp, "\t HWPC_CHOOSER=LOADSTORE:\n");
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\t LOAD_INS:   memory load instructions\n");
	fprintf(fp, "\t\t STORE_INS:  memory store instructions\n");
		if (hwpc_group.i_platform == 2 ) {
	fprintf(fp, "\t\t WBACK_MEM:  memory write via cache writeback store\n");
	fprintf(fp, "\t\t STRMS_MEM:  memory write via streaming store, i.e. nontemporal store \n");
		}
	fprintf(fp, "\t\t [Vector %%]: the measurement of this column is not available on Xeon processors.\n"); 
	} else

	if (hwpc_group.platform == "SPARC64" ) {
		if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
	fprintf(fp, "\t\t LD+ST:      memory load/store instructions\n");
	fprintf(fp, "\t\t SIMD:LDST:  memory load/store SIMD instructions(128bits)\n");
		}
		if (hwpc_group.i_platform == 11 ) {
	fprintf(fp, "\t\t LD+ST:      memory load/store instructions\n");
	fprintf(fp, "\t\t 2SIMD:LDST: memory load/store SIMD instructions(2SIMD)\n");
	fprintf(fp, "\t\t 4SIMD:LDST: memory load/store extended SIMD instructions(4SIMD)\n");
		}
	fprintf(fp, "\t\t [Vector %%]: percentage of SIMD load/store instructions over all load/store instructions.\n");
	} else

	if (hwpc_group.platform == "A64FX" ) {
	fprintf(fp, "\t\t LOAD_INS:   memory load instructions\n");
	fprintf(fp, "\t\t STORE_INS:  memory store instructions\n");
	fprintf(fp, "\t\t SVE_LOAD:   memory read by SVE and Advanced SIMD load instructions.\n");
	fprintf(fp, "\t\t SVE_STORE:  memory write by SVE and Advanced SIMD store instructions.\n");
	fprintf(fp, "\t\t SVE_SMV_LD: memory read by SVE and Advanced SIMD multiple vector contiguous structure load instructions.\n");
	fprintf(fp, "\t\t SVE_SMV_ST: memory write by SVE and Advanced SIMD multiple vector contiguous structure store instructions.\n");
	fprintf(fp, "\t\t GATHER_LD:  memory read by SVE non-contiguous gather-load instructions.\n");
	fprintf(fp, "\t\t SCATTER_ST: memory write by SVE non-contiguous scatter-store instructions.\n");
	fprintf(fp, "\t\t [Vector %%]: percentage of SVE load/store instructions over all load/store instructions.\n");
	}

// CYCLES
	fprintf(fp, "\t HWPC_CHOOSER=CYCLE:\n");
	fprintf(fp, "\t\t TOT_CYC:   total cycles\n");
	fprintf(fp, "\t\t TOT_INS:   total instructions\n");
	if (hwpc_group.platform == "A64FX" ) {
	fprintf(fp, "\t\t FP_inst:   floating point instructions\n");
	fprintf(fp, "\t\t FMA_inst:  fused multiply+add instructions\n");
	fprintf(fp, "\t\t [FMA_ins%%]: percentage of FMA instructions over all f.p. instructions\n");
	}
	fprintf(fp, "\t\t [Ins/cyc]: performed instructions per machine clock cycle\n");

// USER
	fprintf(fp, "\t HWPC_CHOOSER=USER:\n");
	fprintf(fp, "\t\t User provided argument values (Arithmetic Workload) are accumulated and reported.\n");


// remarks
	fprintf(fp, "\n");
	fprintf(fp, "\t Remarks.\n");
	fprintf(fp, "\t\t Symbols represent HWPC (hardware performance counter) native and derived events\n");
	fprintf(fp, "\t\t Symbols in [] are frequently used performance metrics which are calculated from these events.\n");
	fprintf(fp, "\t\t The values in the Basic Report section shows the arithmetic mean value of the processes. \n");
	fprintf(fp, "\t\t The values in the Process Report section shows the sum of threads generated by the process. \n");
	fprintf(fp, "\t\t The values in the Thread Report section shows the precise thread level statistics. \n");

	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t Special remark for Intel Xeon memory bandwidth.\n");
	fprintf(fp, "\t\t The memory bandwidth (BW) is based on core events, not on memory controller information.\n");
	//	fprintf(fp, "\t\t The read BW and the write BW must be obtained separately, unfortunately.\n");
	//	fprintf(fp, "\t\t Use HWPC_CHOOSER=BANDWIDTH to report the read BW responding to demand read and prefetch,\n");
	//	fprintf(fp, "\t\t and HWPC_CHOOSER=LOADSTORE for the write BW responding to writeback and streaming-stores.\n");
	fprintf(fp, "\t\t The symbols L3 cache and LLC both refer to the same Last Level Cache.\n");
	fprintf(fp, "\t\t The following data is available for Sandybridge and Ivybridge only.\n");
	fprintf(fp, "\t\t WBACK_MEM:   memory write via writeback store\n");
	fprintf(fp, "\t\t STRMS_MEM:   memory write via streaming store (nontemporal store)\n");
	}

	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t Special remarks for SPARC64*fx memory bandwidth.\n");
	fprintf(fp, "\t\t The memory bandwidth is based on L2 cache events, not on memory controller information, \n");
	fprintf(fp, "\t\t and is calculated as the sum of demand read miss, prefetch miss and writeback reaching memory.\n");
	}

	if (hwpc_group.platform == "A64FX" ) {
	fprintf(fp, "\t Special remarks for A64FX VECTOR report.\n");
	fprintf(fp, "\t\t [FMA_ops %%] is a roughly approximated number using the following assumption. \n");
	fprintf(fp, "\t\t\t (FMA vector OPS)/(vector OPS) = (FMA scalar OPS)/(scalar OPS) for both DP and SP \n");
	fprintf(fp, "\t Special remarks for A64FX BANDWIDTH report.\n");
	fprintf(fp, "\t\t CMG_bus_RD and CMG_bus_WR both count the CMG aggregated values, not core.\n");
	fprintf(fp, "\t\t So, Thread Report statistics shows the values in redundant manner.\n");
	fprintf(fp, "\t\t Basic Report and Process Report statistics both show the measured value.\n");
	}

#endif // USE_PAPI
}


// Special code block for Fugaku A64FX follows.

void PerfWatch::identifyARMplatform (void)
{
#ifdef USE_PAPI
	// on ARM, PAPI_get_hardware_info() does not provide so useful information.
	// so we use /proc/cpuinfo information instead

	// as of 2020/7/1, PAPI_get_hardware_info() output on Fugaku A64FX is as follows
		//	hwinfo->vendor			// 7
		//	hwinfo->vendor_string	// "ARM"
		//	hwinfo->model			// 0
		//	hwinfo->model_string	// ""
		//	hwinfo->cpuid_family	// 0
		//	hwinfo->cpuid_model		// 1
		//	hwinfo->cpuid_stepping	// 0

	// Fugaku /proc/cpuinfo contains
		//	CPU implementer : 0x46
		//	CPU architecture : 8
		//	CPU variant : 0x0
		//	CPU part : 0x001
		//	CPU revision    : 0
		// Fujitsu says implementer(0x46) and part(0x001) identifies A64FX

	FILE *fp;
	int cpu_implementer = 0;
	int cpu_architecture = 0;
	int cpu_variant = 0;
	int cpu_part = 0;
	int cpu_revision = 0;
	char buffer[1024];

	fp = fopen("/proc/cpuinfo","r");
	if (fp == NULL) {
		fprintf(stderr, "*** Error <identifyARMplatform> can not open /proc/cpuinfo \n");
		return;
	}

	// sscanf handles regexp such as: sscanf (buffer, "%[^\t:]", value);
	while (fgets(buffer, 1024, fp) != NULL) {
		if (!strncmp(buffer, "CPU implementer",15)) {
			sscanf(buffer, "CPU implementer\t: %x", &cpu_implementer);
			continue;
		}
		if (!strncmp(buffer, "CPU architecture",16)) {
			sscanf(buffer, "CPU architecture\t: %d", &cpu_architecture);
			continue;
		}
		if (!strncmp(buffer, "CPU variant",11)) {
			sscanf(buffer, "CPU variant\t: %x", &cpu_variant);
			continue;
		}
		if (!strncmp(buffer, "CPU part",8)) {
			sscanf(buffer, "CPU part\t: %x", &cpu_part);
			continue;
		}
		if (!strncmp(buffer, "CPU revision",12)) {
			sscanf(buffer, "CPU revision\t: %d", &cpu_revision);
			//	continue;
		break;
		}
	}
	fclose(fp);
	if ((cpu_implementer == 0x46) && (cpu_part == 1) ) {
		hwpc_group.i_platform = 21;	// A64FX
	} else {
		hwpc_group.i_platform = 99;	// unsupported ARM hardware
	}
	#ifdef DEBUG_PRINT_PAPI
	if (my_rank == 0) {
		fprintf(stderr, "<identifyARMplatform> reads /proc/cpuinfo\n");
		fprintf(stderr, "cpu_implementer=0x%x\n", cpu_implementer);
		fprintf(stderr, "cpu_architecture=%d\n", cpu_architecture);
		fprintf(stderr, "cpu_variant=0x%x\n", cpu_variant);
		fprintf(stderr, "cpu_part=0x%x\n", cpu_part);
		fprintf(stderr, "cpu_revision=%x\n", cpu_revision);
	}
	#endif
	return;
#endif // USE_PAPI
}



} /* namespace pm_lib */

