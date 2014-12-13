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

//@file   PerfCpuType.cpp
//@brief  PMlib - PAPI interface class

// if USE_PAPI is defined, compile this file with openmp option 

#ifdef _PM_WITHOUT_MPI_
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

extern struct pmlib_papi_chooser papi;
extern struct hwpc_group_chooser hwpc_group;

namespace pm_lib {


  /// HWPC interface initialization
  ///
  /// @note  PMlib - HWPC PAPI interface class
  /// PAPI library is used to interface HWPC events
  ///
void PerfWatch::initializeHWPC ()
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
	fprintf (stderr, "\n <initializeHWPC> starts n_threads=%d, &papi=%llu \n\n", n_thread, &papi.num_events);
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
	fprintf (stderr, " <initializeHWPC> ends\n\n");
	#endif
#endif // USE_PAPI
}



  /// Construct the available list of PAPI counters for the targer processor
  /// @note  this routine needs the processor hardware information
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
	#ifdef DEBUG_PRINT_PAPI
	fprintf (stderr, " <createPapiCounterList> starts\n" );
	#endif
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

// 1. Identify the CPU architecture

	hwinfo = PAPI_get_hardware_info();
	if (hwinfo == NULL) {
		fprintf (stderr, "*** error. <PAPI_get_hardware_info> failed.\n" );
	}

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
		// Bandwidth related event counting with PAPI on Xeon E5 is quite complex,
		// If offcore events can be measured, the bandwidth should be calculated as
		// sum of offcore(demand read/write, prefetch, write back) * linesize *1.0e-9 / time
		// But they are not available in the current environment.
		// Linux kernel must be 3.5+ to support offcore events "OFFCORE_REQUESTS:*"
		// Instead, the bandwidth from/to uncore, i.e. L3 and DRAM, is calculated below
		// The events can be of demand read, rfo, or prefetch, and they must be combined.
		// 	for example, LLC_MISSES represents the L3 miss of demand requests only .
		// For more info regarding the events, see check_papi_events.cpp

			hwpc_group.number[I_bandwidth] += 8;
			papi.s_name[ip] = "LD_INS"; papi.events[ip] = PAPI_LD_INS; ip++;
			papi.s_name[ip] = "SR_INS"; papi.events[ip] = PAPI_SR_INS; ip++;

			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:L1_HIT";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "MEM_LOAD_UOPS_RETIRED:HIT_LFB";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); ip++;
			papi.s_name[ip] = "L2_RQSTS:ALL_DEMAND_DATA_RD";	// L2 demand read request
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_DRD_REQ"; ip++;
			papi.s_name[ip] = "L2_RQSTS:ALL_DEMAND_RD_HIT";		// L2 demand read hit
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_DRD_HIT"; ip++;
			papi.s_name[ip] = "L2_RQSTS:PF_MISS";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_PF_MISS"; ip++;
			papi.s_name[ip] = "L2_RQSTS:RFO_MISS";
			my_papi_name_to_code( papi.s_name[ip].c_str(), &papi.events[ip]); papi.s_name[ip] = "L2_RFO_MISS"; ip++;
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
			//	papi.s_name[ip] = "LST_INS"; papi.events[ip] = PAPI_LST_INS; ip++;
			//	papi.s_name[ip] = "MEM_SCY"; papi.events[ip] = PAPI_MEM_SCY; ip++;
			//	papi.s_name[ip] = "STL_ICY"; papi.events[ip] = PAPI_STL_ICY; ip++;
		}
	}

// total number of traced events by PMlib
	papi.num_events = ip;

// end of hwpc_group selection

	#ifdef DEBUG_PRINT_PAPI
	if (my_id == 0) {
		fprintf(stderr, " platform: %s\n", hwpc_group.platform.c_str());
		fprintf(stderr, " HWPC_CHOOSER=%s\n", s_chooser.c_str());
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
	}
	#endif
#endif // USE_PAPI
}



  /// Sort out the list of counters linked with the user input parameter
  ///
void PerfWatch::sortPapiCounterList (void)
{
#ifdef USE_PAPI

	int ip,jp;
	double flops, counts, bandwidth;

	jp=0;

// if (FLOPS)
	if ( hwpc_group.number[I_flops] > 0 ) {
		flops=0.0;
		ip = hwpc_group.index[I_flops];
		for(int i=0; i<hwpc_group.number[I_flops]; i++)
		{
			papi.s_sorted[jp] = papi.s_name[ip] ;
			flops += papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
		papi.s_sorted[jp] = "[Flops]" ;
		papi.v_sorted[jp] = flops / m_time ; // * 1.0e-9;
		jp++;
	}

// if (VECTOR)
	if ( hwpc_group.number[I_vector] > 0 ) {
		ip = hwpc_group.index[I_vector];
		for(int i=0; i<hwpc_group.number[I_vector]; i++)
		{
			papi.s_sorted[jp] = papi.s_name[ip] ;
			counts += papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
	}

// if (BANDWIDTH)
	if ( hwpc_group.number[I_bandwidth] > 0 ) {
		counts = 0.0;
		ip = hwpc_group.index[I_bandwidth];
		for(int i=0; i<hwpc_group.number[I_bandwidth]; i++)
			{
			papi.s_sorted[jp] = papi.s_name[ip] ;
			counts += papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
			}
    	if (hwpc_group.platform == "Xeon" ) {
			// See notes in createPapiCounterList ()
			// Xeon E5 has 64 Byte L2 $ lines
			// BandWidth = (L2 demad read miss + L2 RFO(write) miss + L2 prefetch miss + L2 WB) x 64 *1.0e-9 / time
			counts = \
				(papi.v_sorted[jp-4]-papi.v_sorted[jp-3]
				+ papi.v_sorted[jp-1]
				+ papi.v_sorted[jp-2]
				+ 0 // WB is not measured...
				);
			bandwidth = counts*64.0/m_time;
		}

    	if (hwpc_group.platform == "SPARC64" ) {
			// BandWidth = (L2_MISS_DM + L2_MISS_PF + L2_WB_DM + L2_WB_PF) x 128 *1.0e-9 / time
			bandwidth = counts * 128 / m_time; // Sparc64 has 128 Byte $ line
		}

		papi.s_sorted[jp] = "[HW B/s]" ;
		papi.v_sorted[jp] = bandwidth ; //* 1.0e-9;
		jp++;
	}

// if (CACHE)
	if ( hwpc_group.number[I_cache] > 0 ) {
		ip = hwpc_group.index[I_cache];
		for(int i=0; i<hwpc_group.number[I_cache]; i++)
		{
			papi.s_sorted[jp] = papi.s_name[ip] ;
			if ( papi.s_sorted[jp] == "OFFCORE_REQUESTS:ALL_DATA_RD" ) papi.s_sorted[jp] = "OFFCORE";
			counts += papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
	}

// if (CYCLE)
	if ( hwpc_group.number[I_cycle] > 0 ) {
		ip = hwpc_group.index[I_cycle];
		for(int i=0; i<hwpc_group.number[I_cycle]; i++)
		{
			papi.s_sorted[jp] = papi.s_name[ip] ;
			counts += papi.v_sorted[jp] = my_papi.accumu[ip] ;
			ip++;jp++;
		}
	}

// count the number of reported events and derived matrices
	papi.num_sorted = jp;

#endif // USE_PAPI
}


  /// Display the HWPC header line
  ///   @param[in] fp 出力ファイルポインタ
  ///   @param[in] s_label ラベル
  ///
void PerfWatch::outputPapiCounterHeader (FILE* fp, std::string s_label)
{
#ifdef USE_PAPI

// print the Label string
	fprintf(fp, "Label   %s%s\n", m_exclusive ? "" : "*", s_label.c_str());

// print the header line with event names
	std::string s;
	int ip, jp, kp;
	fprintf (fp, "Header  ID :");
	for(int i=0; i<papi.num_sorted; i++) {
		kp = papi.s_sorted[i].find_last_of(':');
		if ( kp < 0) {
			s = papi.s_sorted[i];
		} else {
			s = papi.s_sorted[i].substr(kp+1);
		}
		fprintf (fp, " %10.10s", s.c_str() );
	} fprintf (fp, "\n");

#endif // USE_PAPI
}


  /// Display the HWPC measured values and their derived values
  ///   @param[in] fp 出力ファイルポインタ
  ///
void PerfWatch::outputPapiCounterList (FILE* fp)
{
#ifdef USE_PAPI
	double* gather_sorted;
	int iret;
	int m_np;
	MPI_Comm_size(MPI_COMM_WORLD, &m_np);

	iret = MPI_Barrier(MPI_COMM_WORLD);
	if (!(gather_sorted  = new double[m_np * papi.num_sorted])) {
		fprintf (fp, "  gather_sorted failed.\n");
		PM_Exit(0);
	}
	iret =
	MPI_Gather (papi.v_sorted, papi.num_sorted, MPI_DOUBLE,
				gather_sorted, papi.num_sorted, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	if ( iret != 0 ) {
		fprintf (fp, "  *** MPI_Gather failed.\n");
		PM_Exit(0);
	}

// print the event values and their derived values
	if (my_rank == 0) {
	for (int i=0; i<m_np; i++) {
		fprintf(fp, "Rank %5d :", i);

		for(int n=0; n<papi.num_sorted; n++) {
			//	fprintf (fp, "  %9.3e", fabs(papi.v_sorted[n]) );
			fprintf (fp, "  %9.3e", fabs(gather_sorted[i*papi.num_sorted + n]));
		}
		fprintf (fp, "\n");
	}
	}
	iret = MPI_Barrier(MPI_COMM_WORLD);
#endif // USE_PAPI
}


  /// Display the HWPC legend
  ///   @param[in] fp 出力ファイルポインタ
  ///
void PerfWatch::outputPapiCounterLegend (FILE* fp)
{
#ifdef USE_PAPI

	const PAPI_hw_info_t *hwinfo = NULL;
	std::string s_model_string;
	using namespace std;

	fprintf(fp, "\n\t\tHWPC events legend: \n");
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

	fprintf(fp, "\n\t\tPlease note that only the available HWPC events on this CPU are listed.\n");

#endif // USE_PAPI
}


  /// Return the sum of flop values
  ///   @param[in] my_papi struct pmlib_papi_chooser
  ///
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


  /// Debug. This routine needs more testing. It is commented out temporarily.
  ///
  /// Return the byte counts, not the bandwidth
  ///   @param[in] my_papi struct pmlib_papi_chooser
  ///
double PerfWatch::countPapiByte (pmlib_papi_chooser my_papi)
{
	//	double bandwidth;
	double bytes = 0.0;
#ifdef USE_PAPI
	#ifdef DEBUG_PRINT_PAPI
	fprintf (stderr, " <PerfWatch::countPapiByte()> starts\n");
	#endif

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

