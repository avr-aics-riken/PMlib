///
/// @file pmlib_power.h
///
/// @brief header file for calling Power API interface routines
///
///	@note the arguments to the power bind routines are as follows:
///	@verbatim
///		knob  : power knob chooser [0..5]
///		operation : [0:read, 1:update]
///		value     : controlled value
///		pa64timer : internal timer
///		w_joule   : power consumption in Joule
///	@endverbatim
///
///  knob and controlled value combinations
///	@verbatim
///  knob=0 (I_knob_CPU)   : cpu frequency (MHz) : 2200, 2000
///  knob=1 (I_knob_MEMORY): memory access throttling (%) : 100, 90, 80, .. , 10
///  knob=2 (I_knob_ISSUE) : instruction issues (/cycle) : 2, 4
///  knob=3 (I_knob_PIPE)  : number of PIPEs : 1, 2
///  knob=4 (I_knob_ECO)   : eco state (mode) : 0, 1, 2
///  knob=5 (I_knob_RETENTION): retention state (mode) : 0, 1
///		retention at 1600 MHz is not allowed as of 2021 May
///	@endverbatim
///


#include <string>
#include <cstdio>
#include "pwr.h"
#include <cstdlib>

#ifdef USE_POWER
extern int my_power_bind_initialize (void);
extern int my_power_bind_knobs (int knob, int operation, int & value);
extern int my_power_bind_start (uint64_t pa64timer[], double w_joule[]);
extern int my_power_bind_stop (uint64_t pa64timer[], double w_joule[]);
extern int my_power_bind_finalize ();
#endif

//	const int Max_measure_device=1;
//		"plat.node" is the only attribute to get the measured value
//		other attributes return estimated values
//	const int Max_power_leaf_parts=12;
//		max. # of leaf parts in the object group
//		CMG has 12 cores which is the largest number among parts
//	const int Max_power_stats=Max_power_object+Max_power_extended;

const int Max_power_stats=13;

struct pmlib_power_chooser
{
	int num_power_stats;
	uint64_t pa64timer[Max_power_stats];	// typedef uint64_t PWR_Time
	std::string s_name[Max_power_stats];
	double w_timer[Max_power_stats];	// time ==  m_time
	double watt_ave[Max_power_stats];	// Watt average
	double watt_max[Max_power_stats];	// Watt max
	double ws_joule[Max_power_stats];	// Power consumption in (J) == Wh/3600

};

