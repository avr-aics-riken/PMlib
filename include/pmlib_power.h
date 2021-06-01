/// Interface routine for Power API
///
///	@file   power_ext.c
///	@brief  PMlib C functions provide simple monitoring and controlling interface to Power API library
///	@note   current implementation is validated on supercomputer Fugaku
///

#include <string>
#include <cstdio>
#include "pwr.h"
#include <cstdlib>

// ################################################################
// 2021/5/17 K. Mikami
// Testing Power APIs for measured and estimated components
static void error_print(int , std::string , std::string);
static void warning_print (std::string , std::string , std::string );
static void warning_print (std::string , std::string , std::string , int);
// ################################################################


// Objects supported by default context
enum power_object_index
	{
		I_pobj_NODE=0,
		I_pobj_CPU,
		I_pobj_CMG0CORES,
		I_pobj_CMG1CORES,
		I_pobj_CMG2CORES,
		I_pobj_CMG3CORES,
		I_pobj_CMG0L2CACHE,
		I_pobj_CMG1L2CACHE,
		I_pobj_CMG2L2CACHE,
		I_pobj_CMG3L2CACHE,
		I_pobj_ACORES0,
		I_pobj_ACORES1,
		I_pobj_UNCMG,
		I_pobj_TOFU,
		I_pobj_MEM0,
		I_pobj_MEM1,
		I_pobj_MEM2,
		I_pobj_MEM3,
		I_pobj_PCI,
		I_pobj_TOFUOPT,
		Max_power_object
	};

// Objects supported by Fugaku extended context
static char p_obj_name[Max_power_object][30] =
	{
		"plat.node",
		"plat.node.cpu",	// valid for SET only.
		"plat.node.cpu.cmg0.cores",
		"plat.node.cpu.cmg1.cores",
		"plat.node.cpu.cmg2.cores",
		"plat.node.cpu.cmg3.cores",
		"plat.node.cpu.cmg0.l2cache",
		"plat.node.cpu.cmg1.l2cache",
		"plat.node.cpu.cmg2.l2cache",
		"plat.node.cpu.cmg3.l2cache",
		"plat.node.cpu.acores.core0",
		"plat.node.cpu.acores.core1",
		"plat.node.cpu.uncmg",
		"plat.node.cpu.tofu",
		"plat.node.mem0",
		"plat.node.mem1",
		"plat.node.mem2",
		"plat.node.mem3",
		"plat.node.pci",
		"plat.node.tofuopt"
	};
enum power_extended_index
	{
		I_pext_NODE=0,
		I_pext_CMG0CORES,
		I_pext_CMG1CORES,
		I_pext_CMG2CORES,
		I_pext_CMG3CORES,
		I_pext_MEM0,
		I_pext_MEM1,
		I_pext_MEM2,
		I_pext_MEM3,
		Max_power_extended
	};
static char p_ext_name[Max_power_extended][30] =
	{
		"plat.node",		// "plat.node" is the only attribute available to get the measured value
		"plat.node.cpu.cmg0.cores",	//	ditto.
		"plat.node.cpu.cmg1.cores",	//	ditto.
		"plat.node.cpu.cmg2.cores",	//	ditto.
		"plat.node.cpu.cmg3.cores",	//	ditto.
		"plat.node.mem0",	//  can set the value for the estimation
		"plat.node.mem1",	//	ditto.
		"plat.node.mem2",	//	ditto.
		"plat.node.mem3"	//	ditto.
	};

enum power_knob_index
	{
		I_knob_CPU=0,
		I_knob_MEMORY,
		I_knob_ISSUE,
		I_knob_PIPE,
		I_knob_ECO,
		I_knob_RETENTION,
		Max_power_knob
	};

const int Max_measure_device=1;	// "plat.node" is the only attribute available to get the measured value
const int Max_power_leaf_parts=12;	// max. # of leaf parts in the object group, i.e. 12 cores in the CMG.
const int Max_power_stats=Max_power_object+Max_power_extended;

//	PWR_Cntxt pacntxt = NULL;				// typedef void* PWR_Cntxt
//	PWR_Cntxt extcntxt = NULL;
//	PWR_Obj p_obj_array[Max_power_object];		// typedef void* PWR_Obj
//	PWR_Obj p_obj_ext[Max_power_extended];
//	uint64_t u64array[Max_power_leaf_parts];
//	uint64_t u64;

struct pmlib_power_chooser
{
	int num_power_stats;
	PWR_Time pa64timer[Max_power_stats];	// typedef uint64_t
	std::string s_name[Max_power_stats];
	double w_timer[Max_power_stats];	// time ==  m_time
	double watt_ave[Max_power_stats];	// Watt average
	double watt_max[Max_power_stats];	// Watt max
	double ws_joule[Max_power_stats];	// Power consumption in (J) == Wh/3600

};

/*
public:
	int m_is_power
	double p_joule_ave;
	struct power_group_chooser my_power;
private:
	double* p_powerArray;	///< power consumption (Joule)
 */
