#ifndef _PM_PAPI_H_
#define _PM_PAPI_H_

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

/// @file pmlib_papi.h
/// @brief Header block for PMlib - PAPI interface class
///
/// @note The following macro is enabled/disabled via Makefile CFLAGS
///       #define USE_PAPI
///       #define DEBUG_PRINT_PAPI

#ifdef USE_PAPI
#include "papi.h"
extern "C" int my_papi_bind_start ( int *, long long *, int );
extern "C" int my_papi_bind_stop  ( int *, long long *, int );
extern "C" int my_papi_add_events ( int *, int);
extern "C" void my_papi_name_to_code ( const char *, int *);
#endif

#ifdef _OPENMP
	#include <omp.h>
#endif

enum hwpc_output_group {
	I_elapse= 0,
	I_flops,
	I_vector,
	I_bandwidth,
	I_cache,
	I_cycle,
	Max_hwpc_output_group,
};

struct hwpc_group_chooser {
	int number[Max_hwpc_output_group];
	int index[Max_hwpc_output_group];
	int i_platform;		// 1:Intel, 2:SPARC64
	std::string platform;	// "Intel", "SPARC64"
};

const int Max_chooser_events=50;

struct pmlib_papi_chooser {
	int num_events;				// number of PAPI events
	int events[Max_chooser_events];		// PAPI events array
	long long values[Max_chooser_events];	// incremental HW counter values
	long long accumu[Max_chooser_events];	// accumulated HW counter values
	std::string s_name[Max_chooser_events];	// event symbol name
	int num_sorted;			// number of sorted events to report
	double v_sorted[Max_chooser_events];		// sorted event values
	std::string s_sorted[Max_chooser_events];	// sorted event symbols
};

// Macro patches for K computer which has fairly old PAPI 3.6
#if defined(__FUJITSU) || defined(__HPC_ACE__)
#define PAPI_SP_OPS 0	// Needed for compatibility
#define PAPI_DP_OPS 0	// Needed for compatibility
#define PAPI_VEC_SP 0	// Needed for compatibility
#define PAPI_VEC_DP 0	// Needed for compatibility
#endif

#endif // _PM_PAPI_H_
