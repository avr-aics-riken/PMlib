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

#ifdef DISABLE_MPI
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif
#include <cmath>
#include "PerfWatch.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

namespace pm_lib {

  extern struct pmlib_papi_chooser papi;
  extern struct hwpc_group_chooser hwpc_group;


  /// HWPC interface initialization
  ///
  /// @note  PMlib - HWPC PAPI interface class
  /// PAPI library is used to interface HWPC events
  /// this routine is called directly by PerfMonitor class instance
  ///
void PerfWatch::initializeHWPC ()
{
#include <string>

	bool root_in_parallel;
	int root_thread;

	root_in_parallel = false;
	root_thread = 0;
	#ifdef _OPENMP
	root_in_parallel = omp_in_parallel();
	root_thread = omp_get_thread_num();
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

	read_cpu_clock_freq(); /// API for reading processor clock frequency.

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
	//	star:	: Intel(R) Core(TM)2 Duo CPU     E7500  @ 2.93GHz, has sse4
	// vsh-vsp	: Intel(R) Xeon(R) CPU E5-2670 0 @ 2.60GHz	# Sandybridge
	//	eagles:	: Intel(R) Xeon(R) CPU E5-2650 v2 @ 2.60GHz	# SandyBridge
	//	uv01:	: Intel(R) Xeon(R) CPU E5-4620 v2 @ 2.60GHz	# Ivybridge
	//	c01:	: Intel(R) Xeon(R) CPU E5-2640 v3 @ 2.60GHz	# Haswell
	// chicago:	: Intel(R) Xeon(R) CPU E3-1220 v5 @ 3.00GHz (94)# Skylake
	// ito-fep:	: Intel(R) Xeon(R) CPU E7-8880 v4 @ 2.20GHz	# Broadwell
	// water:	: Intel(R) Xeon(R) Gold 6148 CPU @ 2.40GHz	# Skylake
	// fugaku:	: Fujitsu A64FX based on ARM SVE edition @ 2.0 GHz base frequency

	hwinfo = PAPI_get_hardware_info();
	if (hwinfo == NULL) {
		fprintf (stderr, "*** error. <PAPI_get_hardware_info> failed.\n" );
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
    if (s_model_string.find( "Intel" ) != string::npos &&
		s_model_string.find( "Xeon" ) != string::npos ) {
		// hwpc_group.i_platform = 0;	// un-supported platform
		// hwpc_group.i_platform = 1;	// Minimal support for only two types
		// hwpc_group.i_platform = 2;	// Sandybridge and alike platform
		// hwpc_group.i_platform = 3;	// Haswell does not support FLOPS events
		// hwpc_group.i_platform = 4;	// Broadwell. No access to Broadwell yet.
		// hwpc_group.i_platform = 5;	// Skylake and alike platform

		hwpc_group.platform = "Xeon" ;
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
    	else {
			hwpc_group.i_platform = 0;	// un-supported Xeon type
		}
	}

	// Intel Core 2 CPU.
    else if (s_model_string.find( "Intel" ) != string::npos &&
		s_model_string.find( "Core(TM)2" ) != string::npos ) {
		hwpc_group.platform = "Xeon" ;
		hwpc_group.i_platform = 1;	// Minimal support. only two FLOPS types
	}

	// SPARC based processors
    else if (s_model_string.find( "SPARC64" ) != string::npos ) {
		hwpc_group.platform = "SPARC64" ;
    	if ( s_model_string.find( "VIIIfx" ) != string::npos ) {
			hwpc_group.i_platform = 8;	// K computer
		}
    	else if ( s_model_string.find( "IXfx" ) != string::npos ) {
			hwpc_group.i_platform = 9;	// Fujitsu FX10
		}
    	else if ( s_model_string.find( "XIfx" ) != string::npos ) {
			hwpc_group.i_platform = 11;	// Fujitsu FX100
		}
	}

	// ARM based processors
    else if (s_model_string.empty() && s_vendor_string.find( "ARM" ) != string::npos ) {
		// on ARM, PAPI_get_hardware_info() does not provide so useful information.
		// PAPI_get_hardware_info() output on Fugaku A64FX is as follows
		//	hwinfo->vendor			// 7
		//	hwinfo->vendor_string	// "ARM"
		//	hwinfo->model			// 0
		//	hwinfo->model_string	// ""
		//	hwinfo->cpuid_family	// 0
		//	hwinfo->cpuid_model		// 1
		//	hwinfo->cpuid_stepping	// 0
		//
		// so we check /proc/cpuinfo for further information
		//
		identifyARMplatform ();

    	if ( hwpc_group.i_platform == 21 ) {
			hwpc_group.platform = "A64FX" ;
		} else {
			//	hwpc_group.platform = "unknown" ;
			hwpc_group.platform = "unsupported_hardware";
		}
		s_model_string = hwpc_group.platform ;

	#ifdef DEBUG_PRINT_PAPI
		fprintf(stderr, "<createPapiCounterList> called identifyARMplatform()\n");
		fprintf(stderr, " s_model_string=%s\n", s_model_string.c_str());
		fprintf(stderr, " platform: %s\n", hwpc_group.platform.c_str());
	#endif
	}

	// other processors are not supported by PMlib
    else {
			hwpc_group.i_platform = 99;
			hwpc_group.platform = "unsupported_hardware";
	}


// 2. Parse the Environment Variable HWPC_CHOOSER
	string s_chooser;
	string s_default = "USER";
	char* c_env = std::getenv("HWPC_CHOOSER");
	if (c_env != NULL) {
		s_chooser = c_env;
		if (s_chooser == "FLOPS" ||
			s_chooser == "BANDWIDTH" ||
			s_chooser == "VECTOR" ||
			s_chooser == "CACHE" ||
			s_chooser == "CYCLE" ||
			s_chooser == "WRITEBACK" ) {
			;
		} else {
			s_chooser = s_default;
		}
	} else {
		s_chooser = s_default;
	}

	hwpc_group.env_str_hwpc = s_chooser;


// 3. Select the corresponding PAPI hardware counter events
	int ip=0;


// if (FLOPS)
	if ( s_chooser.find( "FLOPS" ) != string::npos ) {
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
///////////////////////////////////////////////////////////
// DEBUG from here
///////////////////////////////////////////////////////////

		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
	//	PAPI_FP_OPS 0x80000066  2   |FP operations|
 	//	|DERIVED_POSTFIX|
 	//	|N0|512|128|/|*|N1|+||
 	//	Native Code[0]: 0x40000022 |FP_SCALE_OPS_SPEC|
 	//	Native Code[1]: 0x40000023 |FP_FIXED_OPS_SPEC|
				//	hwpc_group.number[I_flops] += 1;
				//	papi.s_name[ip] = "FP_OPS"; papi.events[ip] = PAPI_FP_OPS; ip++;
				hwpc_group.number[I_flops] += 2;
				papi.s_name[ip] = "SP_OPS"; papi.events[ip] = PAPI_SP_OPS; ip++;
				papi.s_name[ip] = "DP_OPS"; papi.events[ip] = PAPI_DP_OPS; ip++;
			}
		}
	}

	else
// if (BANDWIDTH)
	if ( s_chooser.find( "BANDWIDTH" ) != string::npos ) {
		hwpc_group.index[I_bandwidth] = ip;
		hwpc_group.number[I_bandwidth] = 0;

		if (hwpc_group.platform == "Xeon" ) {
			// the data transactions from/to memory edge device, i.e. outside Last Level Cache,
			// can be directly obtained with Intel PMU API, but not with PAPI.
			// We choose to use uncore event counter to calculate the memory bandwidth
			// The uncore event naming changed over the PAPI versions and over Xeon generation
			// We do not count RFo events since adding more events cause errors/overflows.

			hwpc_group.number[I_bandwidth] = 2;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;

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
			if (hwpc_group.i_platform == 3 ||
				hwpc_group.i_platform == 5 ) {
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
			}

		} else
		if (hwpc_group.platform == "SPARC64" ) {
			hwpc_group.number[I_bandwidth] += 6;
			// normal load and store counters can not be used together with cache counters
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SIMDLD+ST"; ip++;
			}
			else if (hwpc_group.i_platform == 11 ) {
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "XSIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "XSIMDLD+ST"; ip++;
			}
			//	BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 128 *1.0e-9 / time	// for K and FX10
			//	BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 256 *1.0e-9 / time	// for FX100

			//	SPARC64 event PAPI_L2_TCM == (L2_MISS_DM + L2_MISS_PF)
			//	papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;
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
///////////////////////////////////////////////////////////
// DEBUG from here
///////////////////////////////////////////////////////////
		if (hwpc_group.platform == "A64FX" ) {
			if (hwpc_group.i_platform == 21 ) {
			hwpc_group.number[I_bandwidth] = 2;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;
			}
		}

	}

	else
// if (VECTOR)
	if ( s_chooser.find( "VECTOR" ) != string::npos ) {
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
				;	// no FLOPS support
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
		}
	}

	else
// if (CACHE)

//
// DEBUG from here
//	For calculating cache hit/miss rate,
//	"L2_RQSTS:DEMAND_DATA_RD_HIT" and "L2_RQSTS:PF_HIT" maybe more precise than using PAPI_L2_TCM ...?
//


	if ( s_chooser.find( "CACHE" ) != string::npos ) {
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
		}
	}

	else
// if (CYCLE)
	if ( s_chooser.find( "CYCLE" ) != string::npos ) {
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
	}

	else
// if (WRITEBACK)
	if ( s_chooser.find( "WRITEBACK" ) != string::npos ) {
		hwpc_group.index[I_writeback] = ip;
		hwpc_group.number[I_writeback] = 0;

		if (hwpc_group.platform == "Xeon" ) {
			// WRITEBACK is a special option for memory writeback counting on Xeon processor
			// WRITEBACK option is not available for SPARC since writeback is included in BANDWIDTH option

			hwpc_group.number[I_writeback] = 2;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;

			// memory write operation via writeback and streaming-store
			// The related  events (WB and STRMS) have been deleted for Skylake, for unknown reason.
			if (hwpc_group.i_platform >= 2 && hwpc_group.i_platform <= 4 ) {
				hwpc_group.number[I_writeback] += 2;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:WB:ANY_RESPONSE";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "WBACK_MEM"; ip++;
				papi.s_name[ip] = "OFFCORE_RESPONSE_0:STRM_ST:L3_MISS:SNP_ANY";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "STRMS_MEM"; ip++;
			}
		} else
		if (hwpc_group.platform == "SPARC64" ) {
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				hwpc_group.number[I_writeback] += 2;
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "SIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "SIMDLD+ST"; ip++;
			}
			else if (hwpc_group.i_platform == 11 ) {
				hwpc_group.number[I_writeback] += 2;
				papi.s_name[ip] = "LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LD+ST"; ip++;
				papi.s_name[ip] = "XSIMD_LOAD_STORE_INSTRUCTIONS";
					my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "XSIMDLD+ST"; ip++;
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
		fprintf(stderr, " HWPC_CHOOSER=%s\n", s_chooser.c_str());
		fprintf(stderr, " hwpc max output groups:%d\n", Max_hwpc_output_group);
		for (int i=0; i<Max_hwpc_output_group; i++) {
		fprintf(stderr, "  i:%d hwpc_group.number[i]=%d, hwpc_group.index[i]=%d\n",
						i, hwpc_group.number[i], hwpc_group.index[i]);
		}
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
  /// @note this routine is called from 2 classes.
  /// PerfMonitor::gather() -> gatherHWPC() -> sortPapiCounterList()
  ///   PerfWatch::gather() -> gatherHWPC() -> sortPapiCounterList()
  /// each of the labeled measuring sections call this API
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
		counts=0.0;
		ip = hwpc_group.index[I_flops];
		jp=0;

		for(int i=0; i<hwpc_group.number[I_flops]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		my_papi.s_sorted[jp] = "[Flops]" ;
		//	my_papi.v_sorted[jp] = counts / m_time ; // * 1.0e-9;
		my_papi.v_sorted[jp] = counts * perf_rate;
		jp++;
	}

	else
// if (BANDWIDTH)
	if ( hwpc_group.number[I_bandwidth] > 0 ) {
		double d_load_ins, d_store_ins;
		double d_load_store, d_simd_load_store, d_xsimd_load_store;
		double d_hit_L2, d_miss_L2;
		double d_hit_LLC, d_miss_LLC;
		double bandwidth;

		counts = 0.0;
		ip = hwpc_group.index[I_bandwidth];
		jp=0;
		for(int i=0; i<hwpc_group.number[I_bandwidth]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}

    	if (hwpc_group.platform == "Xeon" ) {
			ip = hwpc_group.index[I_bandwidth];
			d_load_ins  = my_papi.accumu[ip] ;	//	PAPI_LD_INS: "LOAD_INS"
			d_store_ins = my_papi.accumu[ip+1] ;	//	PAPI_SR_INS: "STORE_INS"

			if (hwpc_group.number[I_bandwidth] == 6) {
			d_hit_L2 = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] ;	//	L2 data hit + L2 prefetch hit
			d_hit_LLC = my_papi.accumu[ip+4] ;	//	LLC data read hit + prefetch hit: "L3_HIT"
			d_miss_LLC = my_papi.accumu[ip+5] ;	//	LLC data read miss + prefetch miss: "L3_MISS"

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
			}

		} else
    	if (hwpc_group.platform == "SPARC64" ) {
			ip = hwpc_group.index[I_bandwidth];
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				d_load_store      = my_papi.accumu[ip] ;	//	"LOAD_STORE_INSTRUCTIONS";
				d_simd_load_store = my_papi.accumu[ip+1] ;	//	"SIMD_LOAD_STORE_INSTRUCTIONS";
			// BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 128 *1.0e-9 / time		// K (and FX10) has 128 Byte $ line
				//	counts  = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] + my_papi.accumu[ip+4] ;
				counts  = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] + my_papi.accumu[ip+4] + my_papi.accumu[ip+5] ;
				//	bandwidth = counts * 128 / m_time; // K (and FX10) has 128 Byte $ line
				bandwidth = counts * 128.0 * perf_rate;
			}
			else if (hwpc_group.i_platform == 11 ) {
				d_load_store      = my_papi.accumu[ip] ;	//	"LOAD_STORE_INSTRUCTIONS";
				d_simd_load_store = my_papi.accumu[ip+1] ;	//	"XSIMD_LOAD_STORE_INSTRUCTIONS";
			// BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 256 *1.0e-9 / time		// FX100 has 256 Byte $ line
				//	counts  = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] + my_papi.accumu[ip+4] ;
				counts  = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] + my_papi.accumu[ip+4] + my_papi.accumu[ip+5] ;
				//	bandwidth = counts * 256 / m_time;
				bandwidth = counts * 256.0 * perf_rate;
			}
			my_papi.s_sorted[jp] = "Mem [B/s]" ;
			my_papi.v_sorted[jp] = bandwidth ; //* 1.0e-9;
			jp++;
		}

	}

	else
// if (VECTOR)
	if ( hwpc_group.number[I_vector] > 0 ) {
		double fp_sp1, fp_sp2, fp_sp4, fp_sp8, fp_sp16;
		double fp_dp1, fp_dp2, fp_dp4, fp_dp8, fp_dp16;
		double fp_total, fp_vector;
		double vector_percent;

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
		if (hwpc_group.platform == "Xeon" ) {
			ip = hwpc_group.index[I_vector];
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
				} else {
				vector_percent = 0.0;
				}
			}

		} else
    	if (hwpc_group.platform == "SPARC64" ) {
			ip = hwpc_group.index[I_vector];
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
				} else {
				vector_percent = 0.0;
				}
			}
		}
		my_papi.s_sorted[jp] = "Total_FPs" ;
		my_papi.v_sorted[jp] = fp_total;
		jp++;
		my_papi.s_sorted[jp] = "FLOPS" ;
		//	my_papi.v_sorted[jp] = fp_total / m_time;
		my_papi.v_sorted[jp] = fp_total * perf_rate;
		jp++;
		my_papi.s_sorted[jp] = "VECTOR(%)" ;
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
		if (hwpc_group.platform == "Xeon" ) {
			//	We saved 7 events for Xeon
			ip = hwpc_group.index[I_cache];

			d_load_ins  = my_papi.accumu[ip] ;	//	PAPI_LD_INS
			d_store_ins = my_papi.accumu[ip+1] ;	//	PAPI_SR_INS
			d_hit_L1  = my_papi.accumu[ip+2] ;	//	MEM_LOAD_UOPS_RETIRED:L1_HIT
			d_hit_LFB = my_papi.accumu[ip+3] ;	//	MEM_LOAD_UOPS_RETIRED:HIT_LFB
			d_miss_L1 = my_papi.accumu[ip+4] ;	//	PAPI_L1_TCM
			d_miss_L2 = my_papi.accumu[ip+5] ;	//	PAPI_L2_TCM
			//	d_miss_L3 = my_papi.accumu[ip+6] ;	//	PAPI_L3_TCM
			//	We do not display L3_TCM since it is no use and likely to cause confusion

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
		} else
		if (hwpc_group.platform == "SPARC64" ) {
			ip = hwpc_group.index[I_cache];
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
		}
		my_papi.s_sorted[jp] = "[L1L2hit%]";
		my_papi.v_sorted[jp] = (d_L1_ratio + d_L2_ratio) * 100.0;
		jp++;

		#ifdef DEBUG_PRINT_PAPI
		#pragma omp barrier
		#pragma omp critical
		if (my_rank == 0) {
		fprintf(stderr, "<debug> d_load_ins=%e, d_store_ins=%e, d_hit_L1=%e, d_hit_LFB=%e, d_miss_L1=%e, d_miss_L2=%e, d_cache_transaction=%e, d_L1_ratio=%e, d_L2_ratio=%e\n",
			d_load_ins, d_store_ins, d_hit_L1, d_hit_LFB, d_miss_L1, d_miss_L2, d_cache_transaction, d_L1_ratio, d_L2_ratio);
		}
		#endif
	}

	else
// if (CYCLE)
	if ( hwpc_group.number[I_cycle] > 0 ) {
		ip = hwpc_group.index[I_cycle];
		jp=0;

		//	events[0] = PAPI_TOT_CYC;
		//	events[1] = PAPI_TOT_INS;

		for(int i=0; i<hwpc_group.number[I_cycle] ; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		my_papi.s_sorted[jp] = "[Ins/cyc]" ;
		if ( my_papi.v_sorted[jp-2] > 0.0 ) {
			my_papi.v_sorted[jp] = my_papi.v_sorted[jp-1] / my_papi.v_sorted[jp-2];
		} else {
			my_papi.v_sorted[jp] = 0.0;
		}

		jp++;
	}

	else
// if (WRITEBACK)
	// WRITEBACK is a special option for memory write bandwidth calculation on Intel Xeon Sandybridge and Ivybridge
	// The related events (WB and STRMS) have been deleted on Skylake, for some reason.

	if ( hwpc_group.number[I_writeback] > 0 ) {
		double d_load_ins, d_store_ins;
		double d_writeback_MEM, d_streaming_MEM;
		double bandwidth;
		counts = 0.0;
		ip = hwpc_group.index[I_writeback];
		jp=0;
		for(int i=0; i<hwpc_group.number[I_writeback]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}

    	if (hwpc_group.platform == "Xeon" ) {
			if (hwpc_group.i_platform >= 2 && hwpc_group.i_platform <= 4 ) {
				ip = hwpc_group.index[I_writeback];
				d_writeback_MEM = my_papi.accumu[ip+2] ;	//	OFFCORE_RESPONSE_0:WB:ANY_RESPONSE
				d_streaming_MEM = my_papi.accumu[ip+3] ;	//	OFFCORE_RESPONSE_0:STRM_ST:L3_MISS:SNP_ANY
				bandwidth = (d_writeback_MEM + d_streaming_MEM) * 64.0 * perf_rate;	// Memory write bandwidth
				my_papi.s_sorted[jp] = "Mem [B/s]" ;
				my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
				jp++;
			} else {
				bandwidth = 0.0;
				my_papi.s_sorted[jp] = "not.avail" ;
				my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
				jp++;
			}

		} else
		if (hwpc_group.platform == "SPARC64" ) {
			bandwidth = 0.0;
			my_papi.s_sorted[jp] = "not.avail" ;
			my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
			jp++;
		}
	}
    	
// count the number of reported events and derived matrices
	my_papi.num_sorted = jp;

#ifdef DEBUG_PRINT_PAPI
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



  /// Display the HWPC header lines
  ///
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] s_label ラベル
  ///
  // print the Label string line, and the header line with event names
  //
void PerfWatch::outputPapiCounterHeader (FILE* fp, std::string s_label)
{
#ifdef USE_PAPI

	fprintf(fp, "Label   %s%s\n", m_exclusive ? "" : "*", s_label.c_str());

	std::string s;
	int ip, jp, kp;
	fprintf (fp, "Header  ID :");
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
	using namespace std;

	hwinfo = PAPI_get_hardware_info();
	if (hwinfo == NULL) {
		fprintf (fp, "\n\tHWPC legend is not available. <PAPI_get_hardware_info> failed. \n" );
		return;
	}
	fprintf(fp, "\n\tDetected CPU architecture:\n" );
	fprintf(fp, "\t\t%s\n", hwinfo->vendor_string);
	fprintf(fp, "\t\t%s\n", hwinfo->model_string);
	fprintf(fp, "\t\tThe available HWPC_CHOOSER values and their HWPC events for this CPU are shown below.\n");
	fprintf(fp, "\n");

// CYCLES
	fprintf(fp, "\tHWPC_CHOOSER=CYCLE:\n");
	fprintf(fp, "\t\tTOT_CYC:   total cycles\n");
	fprintf(fp, "\t\tTOT_INS:   total instructions\n");
	fprintf(fp, "\t\t[Ins/cyc]: performed instructions per machine clock cycle\n");

// FLOPS
	fprintf(fp, "\tHWPC_CHOOSER=FLOPS:\n");
	if (hwpc_group.platform == "Xeon" ) {
		if (hwpc_group.i_platform != 3 ) {
	fprintf(fp, "\t\tSP_OPS:    single precision floating point operations\n");
	fprintf(fp, "\t\tDP_OPS:    double precision floating point operations\n");
	fprintf(fp, "\t\t[Flops]:   floating point operations per second \n");
		} else {
	fprintf(fp, "\t\t* Haswell processor does not have floating point operation counters,\n");
	fprintf(fp, "\t\t* so PMlib does not produce HWPC report for FLOPS and VECTOR groups.\n");
		}
	}
	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\tFP_OPS:    floating point operations\n");
	fprintf(fp, "\t\t[Flops]:   floating point operations per second \n");
	}


// BANDWIDTH
	fprintf(fp, "\tHWPC_CHOOSER=BANDWIDTH:\n");
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\tLOAD_INS:  memory load instructions\n");
	fprintf(fp, "\t\tSTORE_INS: memory store instructions\n");
	fprintf(fp, "\t\tL2_RD_HIT: L2 cache data read hit \n");
	fprintf(fp, "\t\tL2_PF_HIT: L2 cache data prefetch hit \n");
	fprintf(fp, "\t\tL3_HIT:    Last Level Cache data read hit \n");
	fprintf(fp, "\t\tL3_MISS:   Last Level Cache data read miss \n");
	fprintf(fp, "\t\t[L2$ B/s]: L2 cache working bandwidth responding to demand read and prefetch\n");
	fprintf(fp, "\t\t[L3$ B/s]: Last Level Cache bandwidth responding to demand read and prefetch\n");
	fprintf(fp, "\t\t[Mem B/s]: Memory read bandwidth responding to demand read and prefetch\n");
	fprintf(fp, "\t\t         : The write bandwidth must be measured separately. See Remarks.\n");
	}

	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\tLD+ST:     memory load/store instructions\n");
	fprintf(fp, "\t\tXSIMDLD+ST: memory load/store extended SIMD instructions\n");
	fprintf(fp, "\t\tL2_MISS_DM: L2 cache misses by demand request\n");
	fprintf(fp, "\t\tL2_MISS_PF: L2 cache misses by prefetch request\n");
	fprintf(fp, "\t\tL2_WB_DM:  writeback by demand L2 cache misses \n");
	fprintf(fp, "\t\tL2_WB_PF:  writeback by prefetch L2 cache misses \n");
	fprintf(fp, "\t\t[Mem B/s]: Memory bandwidth responding to demand read, prefetch and writeback reaching memory\n");
	}

// VECTOR
	fprintf(fp, "\tHWPC_CHOOSER=VECTOR:\n");
	if (hwpc_group.platform == "Xeon" ) {
		if (hwpc_group.i_platform != 3 ) {
	fprintf(fp, "\t\tSP_SINGLE: single precision f.p. scalar instructions\n");
	fprintf(fp, "\t\tSP_SSE:    single precision f.p. SSE instructions\n");
	fprintf(fp, "\t\tSP_AVX:    single precision f.p. 256-bit AVX instructions\n");
	fprintf(fp, "\t\tSP_AVXW:   single precision f.p. 512-bit AVX instructions\n");
	fprintf(fp, "\t\tDP_SINGLE: double precision f.p. scalar instructions\n");
	fprintf(fp, "\t\tDP_SSE:    double precision f.p. SSE instructions\n");
	fprintf(fp, "\t\tDP_AVX:    double precision f.p. 256-bit AVX instructions\n");
	fprintf(fp, "\t\tDP_AVXW:   double precision f.p. 512-bit AVX instructions\n");
	fprintf(fp, "\t\t[Total_FPs]: floating point operations as the sum of instructions*width \n");
	fprintf(fp, "\t\t[Flops]:    floating point operations per second \n");
	fprintf(fp, "\t\t[Vector %]: percentage of vectorized f.p. operations\n");
		} else {
	fprintf(fp, "\t\t Haswell processor does not have floating point operation counters,\n");
	fprintf(fp, "\t\t so PMlib does not produce full HWPC report for FLOPS and VECTOR groups.\n");
		}
	}
	if (hwpc_group.platform == "SPARC64" ) {
		if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
	fprintf(fp, "\t\tFP_INS:    f.p. instructions\n");
	fprintf(fp, "\t\tFMA_INS:   FMA instructions\n");
	fprintf(fp, "\t\tSIMD_FP:   SIMD f.p. instructions\n");
	fprintf(fp, "\t\tSIMD_FMA:  SIMD FMA instructions\n");
		}
		if (hwpc_group.i_platform == 11 ) {
	fprintf(fp, "\t\t1FP_INS:   1 f.p. op instructions\n");
	fprintf(fp, "\t\t2FP_INS:   2 f.p. ops instructions\n");
	fprintf(fp, "\t\t4FP_INS:   4 f.p. ops instructions\n");
	fprintf(fp, "\t\t8FP_INS:   8 f.p. ops instructions\n");
	fprintf(fp, "\t\t16FP_INS: 16 f.p. ops instructions\n");
		}
	fprintf(fp, "\t\t[Total_FPs]: floating point operations as the sum of instructions*width \n");
	fprintf(fp, "\t\t[Flops]:    floating point operations per second \n");
	fprintf(fp, "\t\t[Vector %]: percentage of vectorized f.p. operations\n");
	}

// CACHE
	fprintf(fp, "\tHWPC_CHOOSER=CACHE:\n");
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\tL1_HIT:    L1 data cache hits\n");
	fprintf(fp, "\t\tLFB_HIT:   Cache Line Fill Buffer hits\n");
	fprintf(fp, "\t\tL1_TCM:    L1 data cache activities leading to line replacements\n");
	fprintf(fp, "\t\tL2_TCM:    L2 cache demand access misses\n");
	//	fprintf(fp, "\t\tL3_TCM: level 3 total cache misses by demand\n");
	fprintf(fp, "\t\t[L1$ hit%]: data access hit(%) in L1 data cache and Line Fill Buffer\n");
	}
	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\tLD+ST:     memory load/store instructions\n");
	fprintf(fp, "\t\t2SIMD:LDST: memory load/store SIMD instructions(2SIMD)\n");
	fprintf(fp, "\t\t4SIMD:LDST: memory load/store extended SIMD instructions(4SIMD)\n");
	fprintf(fp, "\t\tL1_TCM:    L1 cache misses (by demand and by prefetch)\n");
	fprintf(fp, "\t\tL2_TCM:    L2 cache misses (by demand and by prefetch)\n");
	fprintf(fp, "\t\t[L1$ hit%]: data access hit(%) in L1 cache \n");
	}
	fprintf(fp, "\t\t[L2$ hit%]: data access hit(%) in L2 cache\n");
	fprintf(fp, "\t\t[L1L2hit%]: sum of hit(%) in L1 and L2 cache\n");

// WRITEBACK
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\tHWPC_CHOOSER=WRITEBACK:\n");
	fprintf(fp, "\t\tThis option is only available for Sandybridge and Ivybridge\n");
	fprintf(fp, "\t\tWBACK_MEM:   memory write via writeback store\n");
	fprintf(fp, "\t\tSTRMS_MEM:   memory write via streaming store (nontemporal store)\n");
	fprintf(fp, "\t\t[Mem B/s]: Memory write bandwidth responding to writeback and streaming-stores\n");
	}

// remarks
	fprintf(fp, "\n");
	fprintf(fp, "\tRemarks.\n");
	fprintf(fp, "\t\t Symbols represent HWPC (hardware performance counter) native and derived events\n");
	fprintf(fp, "\t\t Symbols in [] such as [Ins/cyc] are calculated statistics in shown unit.\n");

	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\tSpecial remark for Intel Xeon memory bandwidth.\n");
	fprintf(fp, "\t\t The memory bandwidth (BW) is based on uncore events, not on memory controller information.\n");
	fprintf(fp, "\t\t The read BW and the write BW must be obtained separately, unfortunately.\n");
	fprintf(fp, "\t\t Use HWPC_CHOOSER=BANDWIDTH to report the read BW responding to demand read and prefetch,\n");
	fprintf(fp, "\t\t and HWPC_CHOOSER=WRITEBACK for the write BW responding to writeback and streaming-stores.\n");
	fprintf(fp, "\t\t The symbols L3 cache and LLC both refer to the same Last Level Cache.\n");
	}

	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\tSpecial remarks for SPARC64*fx memory bandwidth.\n");
	fprintf(fp, "\t\t The memory bandwidth is based on L2 cache events, not on memory controller information, \n");
	fprintf(fp, "\t\t and is calculated as the sum of demand read miss, prefetch miss and writeback reaching memory.\n");
	}

#endif // USE_PAPI
}


// Special code block for Fugaku A64FX follows.

int PerfWatch::identifyARMplatform (void)
{
#ifdef USE_PAPI
	// on ARM, PAPI_get_hardware_info() does not provide so useful information.
	// so we use /proc/cpuinfo information instead

	// PAPI_get_hardware_info() output on Fugaku A64FX is as follows
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
	#ifdef DEBUG_PRINT_PAPI
		fprintf(stderr, "<identifyARMplatform> reads /proc/cpuinfo\n");
		fprintf(stderr, "cpu_implementer=0x%x\n", cpu_implementer);
		fprintf(stderr, "cpu_architecture=%d\n", cpu_architecture);
		fprintf(stderr, "cpu_variant=0x%x\n", cpu_variant);
		fprintf(stderr, "cpu_part=0x%x\n", cpu_part);
		fprintf(stderr, "cpu_revision=%x\n", cpu_revision);
	#endif
	if ((cpu_implementer == 0x46) && (cpu_part == 1) ) {
		hwpc_group.i_platform = 21;	// A64FX
	} else {
		hwpc_group.i_platform = 99;	// A64FX
	}
	return;
#endif // USE_PAPI
}



} /* namespace pm_lib */

