/*
###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2018 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2018 Research Institute for Information Technology(RIIT), Kyushu University.
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

	for (int i=0; i<Max_hwpc_output_group; i++) {
		hwpc_group.number[i] = 0;
		hwpc_group.index[i] = -999999;
		}

	papi.num_events = 0;
	for (int i=0; i<Max_chooser_events; i++){
		papi.events[i] = 0;
		papi.values[i] = 0;
		papi.accumu[i] = 0;
		}

	read_cpu_clock_freq(); /// API for reading processor clock frequency.

#ifdef USE_PAPI
	int i_papi;
	i_papi = PAPI_library_init( PAPI_VER_CURRENT );
	if (i_papi != PAPI_VER_CURRENT ) {
		fprintf (stderr, "*** error. <PAPI_library_init> return code: %d\n", i_papi);
		fprintf (stderr, "\t It should match %lu\n", (unsigned long)PAPI_VER_CURRENT);
		PM_Exit(0);
		return;
		}

	createPapiCounterList ();

	#ifdef DEBUG_PRINT_PAPI
	int my_id;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	int *ip_debug;
	ip_debug=&papi.num_events;
	if (my_id == 0) {
		fprintf(stderr, "<initializeHWPC> papi.num_events=%d, ip_debug=%p\n",
			papi.num_events, ip_debug );
	}
	#endif

	i_papi = PAPI_thread_init( (unsigned long (*)(void)) (omp_get_thread_num) );
	if (i_papi != PAPI_OK ) {
		fprintf (stderr, "*** error. <PAPI_thread_init> failed. %d\n", i_papi);
		PM_Exit(0);
		return;
		}
	#pragma omp parallel
	{
	int t_papi;
	t_papi = my_papi_add_events (papi.events, papi.num_events);
	if ( t_papi != PAPI_OK ) {
		fprintf(stderr, "*** error. <initializeHWPC> <my_papi_add_events> code: %d\n"
			"\t most likely un-supported HWPC PAPI combination. HWPC is terminated.\n" , t_papi);
		papi.num_events = 0;
		PM_Exit(0);
		}
	// In general the arguments to <my_papi_*> should be thread private
	// Here we use shared object, since <> does not change its content
	t_papi = my_papi_bind_start (papi.values, papi.num_events);
	if ( t_papi != PAPI_OK ) {
		fprintf(stderr, "*** error. <initializeHWPC> <my_papi_bind_start> code: %d\n", t_papi);
		PM_Exit(0);
		}
	}

#endif // USE_PAPI
}



  /// Construct the available list of PAPI counters for the targer processor
  /// @note  this routine needs the processor hardware information
  ///
  /// this routine is called by PerfMonitor class instance
  ///
void PerfWatch::createPapiCounterList ()
{
#ifdef USE_PAPI
// Set PAPI counter events. The events are CPU hardware dependent

	const PAPI_hw_info_t *hwinfo = NULL;
	std::string s_model_string;
	using namespace std;
	int my_id;
	int i_papi;

	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	#ifdef DEBUG_PRINT_PAPI
	if (my_id == 0) {
		fprintf(stderr, " <createPapiCounterList> starts\n" );
	}
	#endif

// 1. Identify the CPU architecture

	hwinfo = PAPI_get_hardware_info();
	if (hwinfo == NULL) {
		fprintf (stderr, "*** error. <PAPI_get_hardware_info> failed.\n" );
	}

// ToDo: Create more robust API than PAPI_get_hardware_info(), and replace it.

//	star:	: Intel(R) Core(TM)2 Duo CPU     E7500  @ 2.93GHz, has sse4
// vsh-vsp	: Intel(R) Xeon(R) CPU E5-2670 0 @ 2.60GHz	# Sandybridge
//	eagles:	: Intel(R) Xeon(R) CPU E5-2650 v2 @ 2.60GHz	# SandyBridge
//	uv01:	: Intel(R) Xeon(R) CPU E5-4620 v2 @ 2.60GHz	# Ivybridge
//	c01:	: Intel(R) Xeon(R) CPU E5-2640 v3 @ 2.60GHz	# Haswell
// chicago:	: Intel(R) Xeon(R) CPU E3-1220 v5 @ 3.00GHz (94)# Skylake
// ito-fep:	: Intel(R) Xeon(R) CPU E7-8880 v4 @ 2.20GHz	# Broadwell
// water:	: Intel(R) Xeon(R) Gold 6148 CPU @ 2.40GHz	# Skylake


//	with Linux, s_model_string is taken from "model name" in /proc/cpuinfo
	s_model_string = hwinfo->model_string;

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

			hwpc_group.i_platform = 2;	// Xeon default model : Sandybridge

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
    if (s_model_string.find( "Intel" ) != string::npos &&
		s_model_string.find( "Core(TM)2" ) != string::npos ) {
		hwpc_group.platform = "Xeon" ;
		hwpc_group.i_platform = 1;	// Minimal support. only two FLOPS types
	}

	// SPARC based processors
    if (s_model_string.find( "SPARC64" ) != string::npos ) {
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


// 2. Parse the Environment Variable HWPC_CHOOSER
	string s_chooser;
	string s_default = "NONE (default)";

	char* c_env = std::getenv("HWPC_CHOOSER");
	if (c_env != NULL) {
		s_chooser=c_env;
	} else {
		s_chooser=s_default;
	}

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
		}
	}

// if (BANDWIDTH)
	if ( s_chooser.find( "BANDWIDTH" ) != string::npos ) {
		hwpc_group.index[I_bandwidth] = ip;
		hwpc_group.number[I_bandwidth] = 0;

		if (hwpc_group.platform == "Xeon" ) {
			// The difference of the event categories over the Xeon generations has made it difficult to
			// trace the data transactions from/to memory edge device.
			// Some events must be counted exclusively each other.
			// We decided to trace the Level2 data cache transactions and bandwidth

			hwpc_group.number[I_bandwidth] += 7;
			papi.s_name[ip] = "LOAD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "STORE_INS"; papi.events[ip] = PAPI_SR_INS; ip++;

			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:L1_HIT";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L1_HIT"; ip++;
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:HIT_LFB";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LFB_HIT"; ip++;
			papi.s_name[ip] = "PAPI_L1_TCM";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L1_TCM"; ip++;
			papi.s_name[ip] = "PAPI_L2_TCM";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_TCM"; ip++;

			if (hwpc_group.i_platform == 2 ) {
				papi.s_name[ip] = "L2_TRANS:ALL";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_TRANS"; ip++;
			} else
			if (hwpc_group.i_platform == 3 ) {
				papi.s_name[ip] = "L2_TRANS:ALL_REQUESTS";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_TRANS"; ip++;
			} else
			if (hwpc_group.i_platform == 4 ) {
				// Broadwell can count only 4 cache related events at a time.
				papi.s_name[ip] = "L2_TRANS:ALL_REQUESTS";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_TRANS"; ip++;
			} else
			if (hwpc_group.i_platform == 5 ) {
				// Skylake PAPI events should be checked when they become available
				//	"L2_TRANS:ALL" is missing on Skylake. Compromised stats.
				papi.s_name[ip] = "L2_RQSTS:REFERENCES";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_RQSTS"; ip++;
			} else {
				// This is not useful at all. Just putting here to avoid error termination
				papi.s_name[ip] = "LLC_REFERENCES";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "LLC_REF"; ip++;
			}

		} else
		if (hwpc_group.platform == "SPARC64" ) {
			hwpc_group.number[I_bandwidth] += 5;
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

			papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;
			// The following two events are not shown from papi_avail -d command.
			// They show up from papi_event_chooser NATIVE L2_WB_DM (or L2_WB_PF) command.
			papi.s_name[ip] = "L2_WB_DM";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "L2_WB_PF";
				my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
		}

	}

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

// if (CACHE) || if (CYCLE)
// These options are now merged
	if (( s_chooser.find( "CACHE" ) != string::npos ) ||
		( s_chooser.find( "CYCLE" ) != string::npos )) {
		hwpc_group.index[I_cache] = ip;
		hwpc_group.number[I_cache] = 0;

		hwpc_group.number[I_cache] += 2;
		papi.s_name[ip] = "L1_TCM"; papi.events[ip] = PAPI_L1_TCM; ip++;
		papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;

		if (hwpc_group.platform == "Xeon" ) {
			hwpc_group.number[I_cache] += 1;
			papi.s_name[ip] = "L3_TCM"; papi.events[ip] = PAPI_L3_TCM; ip++;
		}
		hwpc_group.number[I_cache] += 2;
		papi.s_name[ip] = "TOT_CYC"; papi.events[ip] = PAPI_TOT_CYC; ip++;
		papi.s_name[ip] = "TOT_INS"; papi.events[ip] = PAPI_TOT_INS; ip++;
	}



// total number of traced events by PMlib
	papi.num_events = ip;

// end of hwpc_group selection

	#ifdef DEBUG_PRINT_PAPI
	if (my_id == 0) {
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
		fprintf (stderr, " <createPapiCounterList> ends\n" );
	}
	#endif
#endif // USE_PAPI
}


  /// Sort out the list of counters linked with the user input parameter
  ///
  /// @note this routine is called by PerfWatch class instance
  ///  gather() -> gatherHWPC() -> sortPapiCounterList()
  ///  each of the labeled measuring sections call this API
  ///

void PerfWatch::sortPapiCounterList (void)
{
#ifdef USE_PAPI

	int ip,jp;
	double counts, flops, bandwidth;
		double fp_sp1, fp_sp2, fp_sp4, fp_sp8, fp_sp16;
		double fp_dp1, fp_dp2, fp_dp4, fp_dp8, fp_dp16;
		double fp_total, fp_vector;
		double vector_percent;

	jp=0;

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
		my_papi.v_sorted[jp] = counts / m_time ; // * 1.0e-9;
		jp++;
	}

// if (BANDWIDTH)
	if ( hwpc_group.number[I_bandwidth] > 0 ) {
			double d_load_ins, d_store_ins;
			double d_load_store, d_simd_load_store, d_xsimd_load_store;
			double d_hit_LFB, d_hit_L1, d_miss_L1, d_miss_L2, d_all_L2;
			double d_L1_ratio, d_L2_ratio;

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

			//	We saved 7 events for Xeon
			ip = hwpc_group.index[I_bandwidth];

			d_load_ins  = my_papi.accumu[ip] ;	//	PAPI_LD_INS
			d_store_ins = my_papi.accumu[ip+1] ;	//	PAPI_SR_INS
			d_hit_L1  = my_papi.accumu[ip+2] ;	//	MEM_LOAD_UOPS_RETIRED:L1_HIT
			d_hit_LFB = my_papi.accumu[ip+3] ;	//	MEM_LOAD_UOPS_RETIRED:HIT_LFB
			d_miss_L1 = my_papi.accumu[ip+4] ;	//	PAPI_L1_TCM
			d_miss_L2 = my_papi.accumu[ip+5] ;	//	PAPI_L2_TCM
			d_all_L2  = my_papi.accumu[ip+6] ;	//	L2_TRANS:ALL all transactions
			d_L1_ratio = (d_hit_LFB + d_hit_L1) / (d_hit_LFB + d_hit_L1 + d_miss_L1) ;
			d_L2_ratio = (d_hit_LFB + d_hit_L1 + d_miss_L1) / (d_hit_LFB + d_hit_L1 + d_miss_L1 + d_miss_L2) ;

			my_papi.s_sorted[jp] = "L1$hit(%)";
			my_papi.v_sorted[jp] = d_L1_ratio * 100.0;
			jp++;
			my_papi.s_sorted[jp] = "L2$hit(%)";
			my_papi.v_sorted[jp] = d_L2_ratio * 100.0;
			jp++;

			// HW sustained L2_BW = 64*d_all_L2/time // Xeon E5 has 64 Byte L2 $ lines
			bandwidth = d_all_L2*64.0/m_time;
			my_papi.s_sorted[jp] = "L2HW [B/s]"; //	"L2$[B/sec]"
			my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
			jp++;

			// Application level effective L2 bandwidth = 64*d_miss_L1/time // Xeon E5 has 64 Byte L2 $ lines
			bandwidth = d_miss_L1*64.0/m_time;
			my_papi.s_sorted[jp] = "L2App[B/s]";	//	"App[B/sec]"  "L2$App_BW";
			my_papi.v_sorted[jp] = bandwidth ;	//	* 1.0e-9;
			jp++;
			ip=ip+7;

		} else
    	if (hwpc_group.platform == "SPARC64" ) {
			ip = hwpc_group.index[I_bandwidth];
			if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
				d_load_store      = my_papi.accumu[ip] ;	//	"LOAD_STORE_INSTRUCTIONS";
				d_simd_load_store = my_papi.accumu[ip+1] ;	//	"SIMD_LOAD_STORE_INSTRUCTIONS";
			// BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 128 *1.0e-9 / time
				counts  = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] + my_papi.accumu[ip+4] ;
				bandwidth = counts * 128 / m_time; // Sparc64 (K and FX10) has 128 Byte $ line
			}
			else if (hwpc_group.i_platform == 11 ) {
				d_load_store      = my_papi.accumu[ip] ;	//	"LOAD_STORE_INSTRUCTIONS";
				d_simd_load_store = my_papi.accumu[ip+1] ;	//	"XSIMD_LOAD_STORE_INSTRUCTIONS";
			// BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 256 *1.0e-9 / time
				counts  = my_papi.accumu[ip+2] + my_papi.accumu[ip+3] + my_papi.accumu[ip+4] ;
				bandwidth = counts * 256 / m_time; // Sparc64 (K and FX10) has 128 Byte $ line
			}
			my_papi.s_sorted[jp] = "Mem [B/s]" ;
			my_papi.v_sorted[jp] = bandwidth ; //* 1.0e-9;
			jp++;
		}

	}

// if (VECTOR)
	if ( hwpc_group.number[I_vector] > 0 ) {
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
				vector_percent = fp_vector/fp_total;
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
				vector_percent = fp_vector/fp_total;
			}
		}
		my_papi.s_sorted[jp] = "Total_FPs" ;
		my_papi.v_sorted[jp] = fp_total;
		jp++;
		my_papi.s_sorted[jp] = "FLOPS" ;
		my_papi.v_sorted[jp] = fp_total / m_time;
		jp++;
		my_papi.s_sorted[jp] = "VECTOR(%)" ;
		my_papi.v_sorted[jp] = vector_percent * 100.0;
		jp++;

	}

// if (CACHE) || if (CYCLE)
// These options are now merged
	if ( hwpc_group.number[I_cache] > 0 ) {
		ip = hwpc_group.index[I_cache];
		for(int i=0; i<hwpc_group.number[I_cache]; i++)
		{
			my_papi.s_sorted[jp] = my_papi.s_name[ip] ;
			counts += my_papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		my_papi.s_sorted[jp] = "Ins/cycle" ;
		my_papi.v_sorted[jp] = my_papi.v_sorted[jp-1] / my_papi.v_sorted[jp-2];
		jp++;
		my_papi.s_sorted[jp] = "Ins/sec" ;
		my_papi.v_sorted[jp] = my_papi.v_sorted[jp-2] / m_time ; // * 1.0e-9;
		jp++;
	}


// count the number of reported events and derived matrices
	my_papi.num_sorted = jp;

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

	const PAPI_hw_info_t *hwinfo = NULL;
	std::string s_model_string;
	using namespace std;

	hwinfo = PAPI_get_hardware_info();
	fprintf(fp, "\n\tDetected CPU architecture:\n" );
	fprintf(fp, "\t\t%s\n", hwinfo->vendor_string);
	fprintf(fp, "\t\t%s\n", hwinfo->model_string);
	fprintf(fp, "\t\tThe available PMlib HWPC events for this CPU are shown below.\n");
	fprintf(fp, "\tHWPC events legend: \n");
// FLOPS
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\tSP_OPS: single precision floating point operations\n");
	fprintf(fp, "\t\tDP_OPS: double precision floating point operations\n");
	}
	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\tFP_OPS: floating point operations\n");
	}

// BANDWIDTH
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\tLOAD_INS: memory load instructions\n");
	fprintf(fp, "\t\tSTORE_INS: memory store instructions\n");
	fprintf(fp, "\t\tL1_HIT: level 1 cache hits\n");
	fprintf(fp, "\t\tLFB_HIT: cache line fill buffer hits\n");
	fprintf(fp, "\t\tL2_HIT: level 2 cache hits\n");
	fprintf(fp, "\t\tL2_TRANS: level 2 cache all transactions\n");
	}
	fprintf(fp, "\t\tL1_TCM: level 1 total cache misses\n");
	fprintf(fp, "\t\tL2_TCM: level 2 total cache misses\n");
	//	fprintf(fp, "\t\tL3_HIT: level 3 cache hit\n");
	//	fprintf(fp, "\t\tL3_TCM: level 3 total cache misses by demand\n");

	if (hwpc_group.platform == "SPARC64" ) {
	fprintf(fp, "\t\tLD+ST: memory load/store instructions\n");
	fprintf(fp, "\t\tSIMDLD+ST: memory load/store SIMD instructions\n");
	fprintf(fp, "\t\tXSIMDLD+ST: memory load/store extended SIMD instructions\n");
	fprintf(fp, "\t\tL2_TCM: level 2 cache misses (by demand and by prefetch)\n");
	fprintf(fp, "\t\tL2_WB_DM: level 2 cache misses by demand with writeback request\n");
	fprintf(fp, "\t\tL2_WB_PF: level 2 cache misses by prefetch with writeback request\n");
	}

// VECTOR
	if (hwpc_group.platform == "Xeon" ) {
	fprintf(fp, "\t\tSP_SINGLE: single precision f.p. scalar instructions\n");
	fprintf(fp, "\t\tSP_SSE: single precision f.p. SSE instructions\n");
	fprintf(fp, "\t\tSP_AVX: single precision f.p. 256-bit AVX instructions\n");
	fprintf(fp, "\t\tSP_AVXW: single precision f.p. 512-bit AVX instructions\n");
	fprintf(fp, "\t\tDP_SINGLE: double precision f.p. scalar instructions\n");
	fprintf(fp, "\t\tDP_SSE: double precision f.p. SSE instructions\n");
	fprintf(fp, "\t\tDP_AVX: double precision f.p. 256-bit AVX instructions\n");
	fprintf(fp, "\t\tDP_AVXW: double precision f.p. 512-bit AVX instructions\n");
	fprintf(fp, "\t\tremark. FMA(fused multiply-add) instructions are counted twice.\n");
	}
	if (hwpc_group.platform == "SPARC64" ) {
		if (hwpc_group.i_platform == 8 || hwpc_group.i_platform == 9 ) {
	fprintf(fp, "\t\tFP_INS: f.p. instructions\n");
	fprintf(fp, "\t\tFMA_INS: FMA instructions\n");
	fprintf(fp, "\t\tSIMD_FP: SIMD f.p. instructions\n");
	fprintf(fp, "\t\tSIMD_FMA: SIMD FMA instructions\n");
		}
		if (hwpc_group.i_platform == 11 ) {
	fprintf(fp, "\t\t1FP_OPS: 1 f.p. op instructions\n");
	fprintf(fp, "\t\t2FP_OPS: 2 f.p. ops instructions\n");
	fprintf(fp, "\t\t4FP_OPS: 4 f.p. ops instructions\n");
	fprintf(fp, "\t\t8FP_OPS: 8 f.p. ops instructions\n");
	fprintf(fp, "\t\t16FP_OPS: 16 f.p. ops instructions\n");
		}
	}

// CACHE || CYCLE
	fprintf(fp, "\t\tTOT_CYC: total cycles\n");
	fprintf(fp, "\t\tTOT_INS: total instructions\n");

	fprintf(fp, "\tDerived statistics:\n");
	fprintf(fp, "\t\t[GFlops]: floating point operations per nano seconds (10^-9)\n");
	fprintf(fp, "\t\t[Mem GB/s]: memory bandwidth in load+store GB/s\n");
	fprintf(fp, "\t\t[VECTOR(%)]: percentage of vectorized operations\n");
	fprintf(fp, "\t\t[Ins/cycle]: performed instructions per machine clock cycle\n");
	fprintf(fp, "\t\t[Ins/sec]: Giga instructions per second \n");

#endif // USE_PAPI
}



} /* namespace pm_lib */
