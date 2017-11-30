#ifndef _PM_PAPI_H_
#define _PM_PAPI_H_

/*
###################################################################################
#
# PMlib - Performance Monitor Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2017 Advanced Institute for Computational Science(AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2016-2017 Research Institute for Information Technology(RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################
 */

/// PMlib private クラスからPAPI low level関数へのインタフェイスC関数
/// extern "C" int my_papi_bind_start ( long long *, int );
/// extern "C" int my_papi_bind_stop  ( long long *, int );
/// extern "C" int my_papi_add_events ( int *, int);
/// extern "C" void my_papi_name_to_code ( const char *, int *);
///
/// @file pmlib_papi.h
/// @brief Header block for PMlib - PAPI interface class
///

#ifdef USE_PAPI
#include "papi.h"
extern "C" int my_papi_bind_start ( long long *, int );
extern "C" int my_papi_bind_stop  ( long long *, int );
extern "C" int my_papi_bind_read  ( long long *, int );
extern "C" int my_papi_add_events ( int *, int);
extern "C" void my_papi_name_to_code ( const char *, int *);
#endif

/// HWPC counter情報の記憶配列
/// struct hwpc_group_chooser{}
/// struct pmlib_papi_chooser{}

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

const int Max_chooser_events=12;

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

// Macro patches for K computer and FX10 which has fairly old PAPI 3.6
#if defined(K_COMPUTER) || defined(FX10)
#define PAPI_SP_OPS 0	// Needed for compatibility
#define PAPI_DP_OPS 0	// Needed for compatibility
#define PAPI_VEC_SP 0	// Needed for compatibility
#define PAPI_VEC_DP 0	// Needed for compatibility
#endif

#endif // _PM_PAPI_H_
