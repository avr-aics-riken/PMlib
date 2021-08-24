
///
/// @file pmlib_power.h
///
/// @brief header file for calling Power API interface routines
///
///
///	@return initialize returns [n:number of reported items, negative:error]
///	@return other routines returns [0:success, non-zero:error]
///
///	@param	knob  : power knob chooser [0..5]
///	@param	operation : [0:read, 1:update]
///	@param	value     : controlled value
///	@param	pa64timer : internal timer
///	@param	w_joule   : power consumption in Joule
///
/// @note valid knob and value combinations are shown below
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
#include <cstdlib>

#ifdef USE_POWER
#include "pwr.h"
#else
// all power object variables are typedef void*
typedef void* PWR_Cntxt ;
typedef void* PWR_Obj ;
#endif

extern int my_power_bind_start(PWR_Cntxt pacntxt, PWR_Cntxt extcntxt,
		PWR_Obj obj_array[], PWR_Obj obj_ext[],
		uint64_t pa64timer[], double w_joule[]);
extern int my_power_bind_stop (PWR_Cntxt pacntxt, PWR_Cntxt extcntxt,
		PWR_Obj obj_array[], PWR_Obj obj_ext[],
		uint64_t pa64timer[], double w_joule[]);

// The following constants and enumerated are defined in power_ext.cpp
//	const int Max_measure_device=1;
//	const int Max_power_leaf_parts=12;
//	enum Max_power_object = 19

const int Max_power_stats=20;	// Max_power_object + Max_measure_device

struct pmlib_power_chooser
{
	PWR_Cntxt pacntxt ;
	PWR_Cntxt extcntxt ;
	PWR_Obj p_obj_array[Max_power_stats];
	PWR_Obj p_obj_ext[Max_power_stats];

	int num_power_stats;				// number of power measured parts
	int level_report;					// level of details in power report
	std::string s_name[Max_power_stats];	// symbols of the parts
	uint64_t pa64timer[Max_power_stats];	// typedef uint64_t PWR_Time
	double u_joule[Max_power_stats];	// temporary Power value (J) at start
	double v_joule[Max_power_stats];	// temporary Power value (J) at stop
	//	note : 1 Joule == 1 Newton x meter == 1 Watt x second
	double w_accumu[Max_power_stats];	// accumulated Power consumption (J)
	double watt_max[Max_power_stats];	// Watt (during the measurement)
	double watt_ave[Max_power_stats];	// Watt (average)
};

