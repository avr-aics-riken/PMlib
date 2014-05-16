/* ##################################################################
 *
 * PMlib - Performance Monitor library
 *
 * Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
 * All rights reserved.
 *
 * Copyright (c) 2012-2014 Advanced Institute for Computational Science, RIKEN.
 * All rights reserved.
 *
 * ###################################################################
 */

//@file   PerfWatch.cpp
//@brief  PerfWatch class

// When compiling this file with USE_PAPI defined, openmp option should be enabled.

#include <mpi.h>
#include <cmath>
#include "PerfWatch.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

extern struct pmlib_papi_chooser papi;
extern struct hwpc_group_chooser hwpc_group;

namespace pm_lib {


void PerfWatch::initializePapi ()
{
#ifdef USE_PAPI
#include <string>

	char* c_env = std::getenv("OMP_NUM_THREADS");
	if (c_env == NULL) {
		//	#ifdef DEBUG_PRINT_PAPI
		int my_id;
		MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
		if (my_id == 0) fprintf (stderr, "\n\t OMP_NUM_THREADS is not defined. Set it as 1.\n");
		//	#endif
		omp_set_num_threads(1);
	}


	#ifdef DEBUG_PRINT_PAPI
	int n_thread = omp_get_max_threads();
	fprintf (stderr, "\n <initializePapi> starts n_threads=%d, &papi=%llu \n\n", n_thread, &papi.num_events);
	#endif

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

	unsigned long l_papi;
	l_papi = PAPI_library_init( PAPI_VER_CURRENT );
	if (l_papi != PAPI_VER_CURRENT ) {
		fprintf (stderr, "*** error. <PAPI_library_init> return code: %lu\n", l_papi);
		fprintf (stderr, "\t It should match %lu\n", (unsigned long)PAPI_VER_CURRENT);
		PM_Exit(0);
		}

	createPapiCounterList ();

	int i_papi;
	i_papi = PAPI_thread_init( (unsigned long (*)(void)) (omp_get_thread_num) );
	if (i_papi != PAPI_OK ) {
		fprintf (stderr, "*** error. <PAPI_thread_init> failed. %d\n", i_papi);
		PM_Exit(0);
		}
#pragma omp parallel
	{
	i_papi = my_papi_add_events (papi.events, papi.num_events);
	if ( i_papi != PAPI_OK ) {
		int i_thread = omp_get_thread_num();
		fprintf(stderr, "*** error. <my_papi_add_events> code:%d thread:%d\n", i_papi, i_thread);
		fprintf(stderr, "\t most likely un-supported PAPI combination. HWPC is terminated.\n");
		PM_Exit(0);
		}
	}

	#ifdef DEBUG_PRINT_PAPI
	fprintf (stderr, " <initializePapi> ends\n\n");
	#endif
#endif // USE_PAPI
}



void PerfWatch::createPapiCounterList ()
{
#ifdef USE_PAPI
// Set PAPI counter events. The events are CPU hardware dependent

	const PAPI_hw_info_t *hwinfo = NULL;
	std::string s_model_string;
	using namespace std;
	int my_id;
	int i_papi;
	#ifdef DEBUG_PRINT_PAPI
	fprintf (stderr, " <createPapiCounterList> starts\n" );
	#endif
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

// 1. Identify the CPU architecture

	hwinfo = PAPI_get_hardware_info();
	if (hwinfo == NULL) {
		fprintf (stderr, "*** error. <PAPI_get_hardware_info> failed.\n" );
	}

	#ifdef DEBUG_PRINT_PAPI
	if (my_id == 0) {
		fprintf(stderr, " <PAPI_get_hardware_info> detected the followings:\n" );
		fprintf(stderr, "\t%s\n", hwinfo->vendor_string);
		fprintf(stderr, "\t%s\n", hwinfo->model_string);
	}
	#endif

	s_model_string = hwinfo->model_string;

    if (s_model_string.find( "Intel" ) != string::npos &&
		s_model_string.find( "Xeon" ) != string::npos )
	// Intel Xeon E5
	{
		hwpc_group.platform = "Xeon" ;
		hwpc_group.i_platform = 1;
	}
    if (s_model_string.find( "Fujitsu" ) != string::npos &&
		s_model_string.find( "SPARC64" ) != string::npos )
	// Kei and Fujitsu FX10
	{
		hwpc_group.platform = "SPARC64" ;
		hwpc_group.i_platform = 2;
	}
	// Add more CPU architecture selection. Later. Sometime...


// 2. Parse the Environment Variable HWPC_CHOOSER
	//	it may contain multiple options such as HWPC_CHOOSER="FLOPS,VECTOR,..."
	string s_chooser;
	string s_default = "NONE (default)";

	char* c_env = std::getenv("HWPC_CHOOSER");
	if (c_env != NULL) {
		s_chooser=c_env;
	} else {
		s_chooser=s_default;
	}
	if (my_id == 0) fprintf(stderr," <PerfWatch::> HWPC_CHOOSER: %s\n", s_chooser.c_str());

// 3. Select the corresponding PAPI hardware counter events
	int ip=0;

// if (FLOPS)
	if ( s_chooser.find( "FLOPS" ) != string::npos ) {
		hwpc_group.index[I_flops] = ip;
		hwpc_group.number[I_flops] = 0;

		/* [Intel Xeon E5]
			PAPI_SP_OPS + PAPI_DP_OPS count all the floating point operations.
			PAPI_DP_OPS  0x80000068
			 Native Code[0]: 0x4000001c |FP_COMP_OPS_EXE:SSE_SCALAR_DOUBLE|
			 Native Code[1]: 0x40000020 |FP_COMP_OPS_EXE:SSE_FP_PACKED_DOUBLE|
			 Native Code[2]: 0x40000021 |SIMD_FP_256:PACKED_DOUBLE| 256-bit packed
			PAPI_SP_OPS  0x80000067
			 Native Code[0]: 0x4000001d |FP_COMP_OPS_EXE:SSE_FP_SCALAR_SINGLE|
			 Native Code[1]: 0x4000001e |FP_COMP_OPS_EXE:SSE_PACKED_SINGLE|
			 Native Code[2]: 0x4000001f |SIMD_FP_256:PACKED_SINGLE| 256-bit packed
			PAPI_FP_OPS == un-packed operations only. useless. (PAPI_FP_OPS == PAPI_FP_INS)
		*/
		if (hwpc_group.platform == "Xeon" ) { // Intel Xeon E5 specific 
			hwpc_group.number[I_flops] += 2;
			papi.s_name[ip] = "SP_OPS"; papi.events[ip] = PAPI_SP_OPS; ip++;
			papi.s_name[ip] = "DP_OPS"; papi.events[ip] = PAPI_DP_OPS; ip++;
		}
		/* [K and FX10]
			PAPI_FP_OPS contains 4 native events. equivalent of (PAPI_FP_INS + PAPI_FMA_INS):
			 Native Code[0]: 0x40000010  |FLOATING_INSTRUCTIONS|
			 Native Code[2]: 0x40000008  |SIMD_FLOATING_INSTRUCTIONS|
			 Native Code[1]: 0x40000011  |FMA_INSTRUCTIONS|
			 Native Code[3]: 0x40000009  |SIMD_FMA_INSTRUCTIONS|
		*/
		if (hwpc_group.platform == "SPARC64" ) {
			hwpc_group.number[I_flops] += 1;
			papi.s_name[ip] = "FP_OPS"; papi.events[ip] = PAPI_FP_OPS; ip++;
		}
	}
// if (VECTOR)
	if ( s_chooser.find( "VECTOR" ) != string::npos ) {
		hwpc_group.index[I_vector] = ip;
		hwpc_group.number[I_vector] = 0;

		if (hwpc_group.platform == "Xeon" ) {
			hwpc_group.number[I_vector] += 2;
			papi.s_name[ip] = "VEC_SP"; papi.events[ip] = PAPI_VEC_SP; ip++;
			papi.s_name[ip] = "VEC_DP"; papi.events[ip] = PAPI_VEC_DP; ip++;
			// on Intel Xeon, PAPI_VEC_INS can not be counted with PAPI_VEC_SP/PAPI_VEC_DP
		}
		if (hwpc_group.platform == "SPARC64" ) {
			hwpc_group.number[I_vector] += 2;
			papi.s_name[ip] = "VEC_INS"; papi.events[ip] = PAPI_VEC_INS; ip++;
			papi.s_name[ip] = "FMA_INS"; papi.events[ip] = PAPI_FMA_INS; ip++;
		}
	}

// if (BANDWIDTH)
	if ( s_chooser.find( "BANDWIDTH" ) != string::npos ) {
		hwpc_group.index[I_bandwidth] = ip;
		hwpc_group.number[I_bandwidth] = 0;

		if (hwpc_group.platform == "Xeon" ) {
		/* Xeon E5 Sandybridge the choices were made out of trial and error...
			// hit or miss counters
			papi.s_name[ip] = "L3_LAT_CACHE:MISS";	// demand request which missed L3
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:L3_MISS";	// ditto
			papi.s_name[ip] = "L1-DCACHE-LOADS";		// N.A. error. L1_DCACHE_LOAD either
			papi.s_name[ip] = "L1-DCACHE-LOADS-MISSES";	// N.A. error.
			papi.s_name[ip] = "PERF_COUNT_HW_CACHE_L1D:ACCESS";	// N.A. error.
		*/
		// offcore requests and offcore response.  OFFCORE_RESPONSE_0:* only return 0
		// OFFCORE_REQUESTS:* return reasonable values,
		// so the bandwidth is calculated using OFFCORE_REQUESTS:ALL_DATA_RD
			hwpc_group.number[I_bandwidth] += 7;

			papi.s_name[ip] = "LD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "SR_INS"; papi.events[ip] = PAPI_SR_INS; ip++;
			//	following events are native events
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:HIT_LFB";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:L1_HIT";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:L2_HIT";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:L3_HIT";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "OFFCORE_REQUESTS:ALL_DATA_RD";	// dm and prefetch missed L3
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
		}

		if (hwpc_group.platform == "SPARC64" ) {
		// load and store counters can not be used together with cache counters
		//	BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 128 *1.0e-9 / time

			hwpc_group.number[I_bandwidth] += 3;
			papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;
			//	PAPI_L2_TCM == (L2_MISS_DM + L2_MISS_PF)
			papi.s_name[ip] = "L2_WB_DM";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "L2_WB_PF";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
		}

	}

// if (CACHE) 
	if ( s_chooser.find( "CACHE" ) != string::npos ) {
		hwpc_group.index[I_cache] = ip;
		hwpc_group.number[I_cache] = 0;

		hwpc_group.number[I_cache] = +2;
		papi.s_name[ip] = "L1_TCM"; papi.events[ip] = PAPI_L1_TCM; ip++;
		papi.s_name[ip] = "L2_TCM"; papi.events[ip] = PAPI_L2_TCM; ip++;

		if (hwpc_group.platform == "Xeon" ) { // Intel Xeon E5 specific 
			hwpc_group.number[I_cache] += 2;
			papi.s_name[ip] = "L3_TCM"; papi.events[ip] = PAPI_L3_TCM; ip++;
			//	following events are native events
			papi.s_name[ip] = "OFFCORE_REQUESTS:ALL_DATA_RD";	// dm and prefetch missed L3
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
		}
	}

// if (CYCLE)
	if ( s_chooser.find( "CYCLE" ) != string::npos ) {
		hwpc_group.index[I_cycle] = ip;
		hwpc_group.number[I_cycle] = 0;

		hwpc_group.number[I_cycle] = +2;
		papi.s_name[ip] = "TOT_CYC"; papi.events[ip] = PAPI_TOT_CYC; ip++;
		papi.s_name[ip] = "TOT_INS"; papi.events[ip] = PAPI_TOT_INS; ip++;

		if (hwpc_group.platform == "SPARC64" ) { // Kei/FX10 Sparc64 specific 
			hwpc_group.number[I_cycle] += 2;
			papi.s_name[ip] = "LD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "SR_INS"; papi.events[ip] = PAPI_SR_INS; ip++;
			//	papi.s_name[ip] = "MEM_SCY"; papi.events[ip] = PAPI_MEM_SCY; ip++;
			//	papi.s_name[ip] = "STL_ICY"; papi.events[ip] = PAPI_STL_ICY; ip++;
		}
	}


	papi.num_events = ip;
// end of hwpc_group selection

#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr, " CPU architecture: %s\n", hwpc_group.platform.c_str());
	fprintf(stderr, " hwpc max output groups:%d\n", Max_hwpc_output_group);
	for (int i=0; i<Max_hwpc_output_group; i++) {
		fprintf(stderr, "  i:%d hwpc_group.number[i]=%d, hwpc_group.index[i]=%d\n",
						i, hwpc_group.number[i], hwpc_group.index[i]);
		}
	fprintf(stderr, " papi.num_events=%d, &papi=%lu \n", papi.num_events, &papi.num_events);
	for (int i=0; i<papi.num_events; i++) {
		fprintf(stderr, " i=%d : %s : events[i]=%u \n", 
					i, papi.s_name[i].c_str(), papi.events[i]);
		}
	fprintf (stderr, " <createPapiCounterList> ends\n" );
#endif // DEBUG_PRINT_PAPI
#endif // USE_PAPI
}



void PerfWatch::outputPapiCounterList (FILE* fp)
{
#ifdef USE_PAPI

	std::string s_value[Max_chooser_events];
	double v_value[Max_chooser_events];
	int ip,jp;
	double flops, counts, bandwidth;

	jp=0;

// if (FLOPS)
	if ( hwpc_group.number[I_flops] > 0 ) {
		flops=0.0;
		ip = hwpc_group.index[I_flops];
		for(int i=0; i<hwpc_group.number[I_flops]; i++)
		{
			s_value[jp] = papi.s_name[ip] ;
			flops += v_value[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		s_value[jp] = "[GFlops]" ;
		v_value[jp] = flops / m_time;	// scale 10.e-9 later
		jp++;
	}

// if (VECTOR)
	if ( hwpc_group.number[I_vector] > 0 ) {
		ip = hwpc_group.index[I_vector];
		for(int i=0; i<hwpc_group.number[I_vector]; i++)
		{
			s_value[jp] = papi.s_name[ip] ;
			counts += v_value[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
	}

// if (BANDWIDTH)
	if ( hwpc_group.number[I_bandwidth] > 0 ) {
		counts = 0.0;
		ip = hwpc_group.index[I_bandwidth];
		for(int i=0; i<hwpc_group.number[I_bandwidth]; i++)
			{
			s_value[jp] = papi.s_name[ip] ;
			if ( s_value[jp] == "OFFCORE_REQUESTS:ALL_DATA_RD" ) s_value[jp] = "OFFCORE";
			counts += v_value[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
			}
    	if (hwpc_group.platform == "Xeon" ) {
			// the last event should be OFFCORE == OFFCORE_REQUESTS:ALL_DATA_RD
			if ( s_value[jp-1] != "OFFCORE" ) {
				fprintf(stderr, "*** internal error. string does not match. %s\n", \
						s_value[jp-1].c_str());
			}
			// Bandwidth = OFFCORE_REQUESTS:ALL_DATA_RD * 64 *1.0e-9 / time
			bandwidth = v_value[jp-1] * 64 / m_time; // Xeon E5 has 64 Byte L3 $ lines
		}
    	if (hwpc_group.platform == "SPARC64" ) {
			// BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 128 *1.0e-9 / time
			bandwidth = counts * 128 / m_time; // Sparc64 has 128 Byte $ line
		}
		s_value[jp] = "[Mem GB/s]" ;
		v_value[jp] = bandwidth;
		jp++;
	}

// if (CACHE)
	if ( hwpc_group.number[I_cache] > 0 ) {
		ip = hwpc_group.index[I_cache];
		for(int i=0; i<hwpc_group.number[I_cache]; i++)
		{
			s_value[jp] = papi.s_name[ip] ;
			if ( s_value[jp] == "OFFCORE_REQUESTS:ALL_DATA_RD" ) s_value[jp] = "OFFCORE";
			counts += v_value[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
	}

// if (CYCLE)
	if ( hwpc_group.number[I_cycle] > 0 ) {
		ip = hwpc_group.index[I_cycle];
		for(int i=0; i<hwpc_group.number[I_cycle]; i++)
		{
			s_value[jp] = papi.s_name[ip] ;
			counts += v_value[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
	}

// Now, print the event values and their derived values
	int kp;
	std::string s;
	int n_thread = omp_get_max_threads();
	fprintf(fp, "\tHardware Performance counter values (rank #0 sum of %d threads) time=%f\n", n_thread, m_time);
	for(int i=0; i<jp; i++) {
		kp = s_value[i].find_last_of(':');
		if ( kp < 0) {
			s = s_value[i];
		} else {
			s = s_value[i].substr(kp);
		}
		fprintf (fp, "  %10.10s", s.c_str() );
	} fprintf (fp, "\n");
	for(int i=0; i<jp; i++) {
			fprintf (fp, "  %10.3f", fabs(v_value[i]*1.0e-9) );
	} fprintf (fp, "\n");

#endif // USE_PAPI
}


void PerfWatch::outputPapiCounterLegend (FILE* fp)
{
#ifdef USE_PAPI

	const PAPI_hw_info_t *hwinfo = NULL;
	std::string s_model_string;
	using namespace std;
	int my_id;
	int i_papi;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

	hwinfo = PAPI_get_hardware_info();
	if (my_id == 0) {
		fprintf(stderr, "\n\t\tThe tool automatically detected the CPU architecture:\n" );
		fprintf(stderr, "\t\t\t%s\n", hwinfo->vendor_string);
		fprintf(stderr, "\t\t\t%s\n", hwinfo->model_string);
	}

	fprintf(fp, "\t\tHardware performance counter events legend: unit in Giga (10^9)\n");
	fprintf(fp, "\t\t\tFP_OPS: floating point operations\n");
	if (hwpc_group.platform == "Xeon" ) { // Intel Xeon E5 specific 
	fprintf(fp, "\t\t\tSP_OPS: single precision floating point operations\n");
	fprintf(fp, "\t\t\tDP_OPS: double precision floating point operations\n");
	fprintf(fp, "\t\t\tVEC_SP: single precision vector floating point operations\n");
	fprintf(fp, "\t\t\tVEC_DP: double precision vector floating point operations\n");
	}
	if (hwpc_group.platform == "SPARC64" ) { // Kei/FX10 Sparc64 specific 
	fprintf(fp, "\t\t\tVEC_INS: vector instructions\n");
	fprintf(fp, "\t\t\tFMA_INS: Fused Multiply-and-Add instructions\n");
	}
	fprintf(fp, "\t\t\tLD_INS: memory load instructions\n");
	fprintf(fp, "\t\t\tSR_INS: memory store instructions\n");
	if (hwpc_group.platform == "Xeon" ) { // Intel Xeon E5 specific 
	fprintf(fp, "\t\t\tL1_HIT: level 1 cache hit\n");
	fprintf(fp, "\t\t\tL2_HIT: level 2 cache hit\n");
	fprintf(fp, "\t\t\tL3_HIT: level 3 cache hit\n");
	fprintf(fp, "\t\t\tHIT_LFB cache line fill buffer hit\n");
	}

	fprintf(fp, "\t\t\tL1_TCM: level 1 cache miss\n");
	if (hwpc_group.platform == "Xeon" ) { // Intel Xeon E5 specific 
	fprintf(fp, "\t\t\tL2_TCM: level 2 cache miss\n");
	fprintf(fp, "\t\t\tL3_TCM: level 3 cache miss by demand\n");
	fprintf(fp, "\t\t\tOFFCORE: demand and prefetch request cache miss \n");
	}
	if (hwpc_group.platform == "SPARC64" ) { // Kei/FX10 Sparc64 specific 
	fprintf(fp, "\t\t\tL2_TCM: level 2 cache miss (by demand and by prefetch)\n");
	fprintf(fp, "\t\t\tL2_WB_DM: level 2 cache miss by demand with writeback request\n");
	fprintf(fp, "\t\t\tL2_WB_PF: level 2 cache miss by prefetch with writeback request\n");
	}
	fprintf(fp, "\t\t\tTOT_CYC: total cycles\n");
	if (hwpc_group.platform == "SPARC64" ) { // Kei/FX10 Sparc64 specific 
	fprintf(fp, "\t\t\tMEM_SCY: Cycles Stalled Waiting for memory accesses\n");
	fprintf(fp, "\t\t\tSTL_ICY: Cycles with no instruction issue\n");
	}

	fprintf(fp, "\t\t\tTOT_INS: total instructions\n");
	fprintf(fp, "\t\t\tFP_INS: floating point instructions\n");

	fprintf(fp, "\t\tDerived statistics:\n");
	fprintf(fp, "\t\t\t[GFlops]: floating point operations per nano seconds (10^-9)\n");
	fprintf(fp, "\t\t\t[Mem GB/s]: memory bandwidth in load+store GB/s\n");
	fprintf(fp, "\t\t\t[L1$ %]: Level 1 cache hit percentage \n");
	fprintf(fp, "\t\t\t[LL$ %]: Last Level cache hit percentage \n");

	fprintf(fp, "\n\t\tPlease note that the available events are platform dependent.\n");

#endif // USE_PAPI
}


double PerfWatch::countPapiFlop (pmlib_papi_chooser my_papi)
{
	double flops=0.0;
#ifdef USE_PAPI
	#ifdef DEBUG_PRINT_PAPI
	fprintf (stderr, " <PerfWatch::countPapiFlop()> starts\n");
	#endif

// if (FLOPS)
	flops = 0.0;
	if ( hwpc_group.number[I_flops] > 0 ) {
		int ip = hwpc_group.index[I_flops];
		for(int i=0; i<hwpc_group.number[I_flops]; i++)
		{
			//	flops += my_papi.accumu[ip++] ;
			flops += my_papi.values[ip++] ;
		}
	}

#endif
	return(flops);
}

double PerfWatch::countPapiByte (pmlib_papi_chooser my_papi)
{
	//	double bandwidth;
	double bytes = 0.0;
#ifdef USE_PAPI
	#ifdef DEBUG_PRINT_PAPI
	fprintf (stderr, " <PerfWatch::countPapiMemBW()> starts\n");
	#endif

// if (BANDWIDTH)
// if (Count bytes instead of bandwidth)
	if ( hwpc_group.number[I_bandwidth] > 0 ) {
		long counts = 0.0;
		int ip = hwpc_group.index[I_bandwidth];
		for(int i=0; i<hwpc_group.number[I_bandwidth]; i++)
			{
			//	counts += my_papi.accumu[ip++] ;
			counts += my_papi.values[ip++] ;
			}
    	if (hwpc_group.platform == "Xeon" ) {
			// the last event should be OFFCORE == OFFCORE_REQUESTS:ALL_DATA_RD
			// Bandwidth = OFFCORE_REQUESTS:ALL_DATA_RD * 64 *1.0e-9 / time
			bytes = my_papi.values[ip-1] * 64 ; // Xeon E5 has 64 Byte L3 $ lines
			//	bandwidth = bytes / m_time;
		}
    	if (hwpc_group.platform == "SPARC64" ) {
			// BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 128 *1.0e-9 / time
			bytes = counts * 128 ; // Sparc64 has 128 Byte $ line
			//	bandwidth = bytes / m_time;
		}
	}
#endif
	return(bytes);
}

} /* namespace pm_lib */

