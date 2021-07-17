
/// Interface routine for Power API
///
///	@file   power_PerfWatch.cpp
///	@brief  PMlib C++ interface functions to simply monitor and controll the Power API library
///	@note   current implementation is validated on supercomputer Fugaku
///

#include <string>
#include <cstdio>
//	#include "pwr.h"
#include <cstdlib>
#include <cmath>

#ifdef DISABLE_MPI
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif

#include "PerfWatch.h"

static void error_print(int , std::string , std::string);
static void warning_print (std::string , std::string , std::string );
static void warning_print (std::string , std::string , std::string , int);

void error_print(int irc, std::string cstr1, std::string cstr2)
{
	fprintf(stderr, "*** PMlib Error. <power_ext::%s> failed. [%s] return code %d \n",
		cstr1.c_str(), cstr2.c_str(), irc);
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3)
{
	fprintf(stderr, "*** PMlib Warning. <power_ext::%s> failed. [%s] %s \n",
		cstr1.c_str(), cstr2.c_str(), cstr3.c_str());
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3, int value)
{
	fprintf(stderr, "*** PMlib Warning. <power_ext::%s> failed. [%s] %s : value %d \n",
		cstr1.c_str(), cstr2.c_str(), cstr3.c_str(), value);
	return;
}
// Objects supported by default context
enum power_object_index
	{
		I_pobj_NODE=0,
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
		I_pobj_TOFU,
		I_pobj_UNCMG,
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
		"plat.node.cpu.tofu",
		"plat.node.cpu.uncmg",
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
		I_pext_CPU,
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
// NODE is the only attribute available to get the measured value
// other extended parts are defined to enable power knob controll
static char p_ext_name[Max_power_extended][30] =
	{
		"plat.node",
		"plat.node.cpu",	// valid both for default and exxtened context
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
		//	I_knob_RETENTION,	// disabled
		Max_power_knob
	};

const int Max_measure_device=1;
	// NODE ("plat.node") is the only attribute to get the power meter measured value from
const int Max_power_leaf_parts=12;
	// max. # of leaf parts in the object group, i.e. 12 cores in the CMG.


namespace pm_lib {


int PerfWatch::my_power_bind_start(PWR_Cntxt pacntxt, PWR_Cntxt extcntxt,
			PWR_Obj p_obj_array[], PWR_Obj p_obj_ext[], uint64_t pa64timer[], double w_joule[])
{

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "<my_power_bind_start> Max_power_object=%d, Max_power_extended=%d\n",
			Max_power_object, Max_power_extended);
	#endif
	int irc;
	if ( (Max_power_object + Max_power_extended) == 0 ) {
		return 0;
	}

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "\t starting objects in default context\n");
	#endif
	for (int i=0; i<Max_power_object; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_array[i], PWR_ATTR_ENERGY, &w_joule[i], &pa64timer[i]);
		if (irc != PWR_RET_SUCCESS)
			{ warning_print ("my_power_bind_start", p_obj_name[i], "default GetValue"); }
	}

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "\t starting objects in extended context\n");
	#endif
	int iadd=Max_power_object;
	//	for (int i=0; i<Max_power_extended; i++) {
	for (int i=0; i<Max_measure_device; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_ext[i], PWR_ATTR_MEASURED_ENERGY, &w_joule[i+iadd], &pa64timer[i+iadd]);
		if (irc != PWR_RET_SUCCESS)
			{ warning_print ("my_power_bind_start", p_ext_name[i], "extended GetValue"); }
	}

	return 0;
}


int PerfWatch::my_power_bind_stop (PWR_Cntxt pacntxt, PWR_Cntxt extcntxt,
			PWR_Obj p_obj_array[], PWR_Obj p_obj_ext[], uint64_t pa64timer[], double w_joule[])
{

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "\t <my_power_bind_stop> Max_power_object=%d, Max_power_extended=%d\n",
			Max_power_object, Max_power_extended);
	#endif
	int irc;
	if ( (Max_power_object + Max_power_extended) == 0 ) {
		return 0;
	}

	for (int i=0; i<Max_power_object; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_array[i], PWR_ATTR_ENERGY, &w_joule[i], &pa64timer[i]);
		if (irc != PWR_RET_SUCCESS)
			{ warning_print ("my_power_bind_stop", p_obj_name[i], "default GetValue"); }
	}

	int iadd=Max_power_object;
	//	for (int i=0; i<Max_power_extended; i++) {
	for (int i=0; i<Max_measure_device; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_ext[i], PWR_ATTR_MEASURED_ENERGY, &w_joule[i+iadd], &pa64timer[i+iadd]);
		if (irc != PWR_RET_SUCCESS)
			{ warning_print ("my_power_bind_stop", p_ext_name[i], "extended GetValue"); }
	}

	return 0;
}

}	// end of namespace pm_lib
