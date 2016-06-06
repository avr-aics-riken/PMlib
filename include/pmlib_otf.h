#ifndef _PM_OTF_H_
#define _PM_OTF_H_

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

/// PMlib private クラスからOTF へのインタフェイスC関数
/// included in PerfWatch.h
/// 
/// @file pmlib_otf.h
/// @brief Header block for PMlib - OTF interface class
///

#ifdef USE_OTF
extern "C" void my_otf_initialize  (int, int, const char*, double);
extern "C" void my_otf_event_start (int, double, int, int);
extern "C" void my_otf_event_stop  (int, double, int, int, double);
extern "C" void my_otf_event_label (int, int, int, const char*, int);
extern "C" void my_otf_finalize    (int, int, int, const char*, const char*, const char*, const char*);
#endif

//	struct otf_group_chooser {
//		int number[Max_otf_output_group];
//		int index[Max_otf_output_group];
//	};

#endif // _PM_OTF_H_
