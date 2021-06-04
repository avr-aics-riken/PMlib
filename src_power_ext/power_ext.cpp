
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
#include <cmath>

static void error_print(int , std::string , std::string);
static void warning_print (std::string , std::string , std::string );
static void warning_print (std::string , std::string , std::string , int);

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
	// NODE ("plat.node") is the only attribute available to get the measured value
const int Max_power_leaf_parts=12;
	// max. # of leaf parts in the object group, i.e. 12 cores in the CMG.

static PWR_Cntxt pacntxt = NULL;				// typedef void* PWR_Cntxt
static PWR_Cntxt extcntxt = NULL;
static PWR_Obj p_obj_array[Max_power_object];		// typedef void* PWR_Obj
static PWR_Obj p_obj_ext[Max_power_extended];
static uint64_t u64array[Max_power_leaf_parts];
static uint64_t u64;


int my_power_bind_initialize (void)
{
	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "<my_power_initialize> objects in default context\n");
	#endif

	int irc;
	int isum;

	isum = 0;
	isum += irc = PWR_CntxtInit (PWR_CNTXT_DEFAULT, PWR_ROLE_APP, "app", &pacntxt);
	if (irc != PWR_RET_SUCCESS) error_print(irc, "PWR_CntxtInit", "default");

	for (int i=0; i<Max_power_object; i++) {
		isum += irc = PWR_CntxtGetObjByName (pacntxt, p_obj_name[i], &p_obj_array[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("CntxtGetObj", p_obj_name[i], "default"); continue;}
	}

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "<my_power_initialize> objects in extended context\n");
	#endif

	isum += irc = PWR_CntxtInit (PWR_CNTXT_FX1000, PWR_ROLE_APP, "app", &extcntxt);
	if (irc != PWR_RET_SUCCESS) error_print(irc, "PWR_CntxtInit", "extended");

	for (int i=0; i<Max_power_extended; i++) {
		isum += irc = PWR_CntxtGetObjByName (extcntxt, p_ext_name[i], &p_obj_ext[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("CntxtGetObj", p_ext_name[i], "extended"); continue;}
	}

	if (isum == 0) {
		return (Max_power_object + Max_measure_device);
	} else {
		return(-1);
	}
}


int my_power_bind_knobs (int knob, int operation, int & value)
{
	//	knob and value combinations
	//		knob=0 : I_knob_CPU    : cpu frequency (MHz)
	//		knob=1 : I_knob_MEMORY : memory access throttling (%) : 100, 90, 80, .. , 10
	//		knob=2 : I_knob_ISSUE  : instruction issues (/cycle) : 2, 4
	//		knob=3 : I_knob_PIPE   : number of PIPEs : 1, 2
	//		knob=4 : I_knob_ECO    : eco state (mode) : 0, 1, 2
	//		knob=5 : I_knob_RETENTION : retention state (mode) : 0, 1
	//	operation : [0:read, 1:update]

	PWR_Grp p_obj_grp = NULL;
	int irc;
	const int reading_operation=0;
	const int update_operation=1;
	double Hz;

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "<my_power_bind_knobs> knob=%d, operation=%d, value=%d\n", knob, operation, value);
	#endif

	if ( knob < 0 | knob >Max_power_knob ) {
		error_print(knob, "my_power_bind_knobs", "invalid controler");
		return(-1);
	}
	if ( operation == update_operation ) {
		for (int i=0; i < Max_power_leaf_parts; i++) {
			u64array[i] = value;
		}
	}

	if ( knob == I_knob_CPU ) {
		// CPU frequency controll can be obtained either from the default context or the extended context
		if ( operation == reading_operation ) {
			irc = PWR_ObjAttrGetValue (p_obj_ext[I_pext_CPU], PWR_ATTR_FREQ, &Hz, NULL);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GetValue", p_ext_name[I_pext_CPU]); return(irc); }
			value = lround (Hz / 1.0e6);
		} else {
			if ( !(value == 2200 || value == 2000) ) {	// 1.6 GHz retention frequency  is not allowed as of 2021 May
				warning_print ("SetValue", p_ext_name[I_pext_CPU], "invalid frequency", value); return(-1);
			}
			Hz = (double)value * 1.0e6;
			irc = PWR_ObjAttrSetValue (p_obj_ext[I_pext_CPU], PWR_ATTR_FREQ, &Hz);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "SetValue", p_ext_name[I_pext_CPU]); return(irc); }
		}
	} else
	if ( knob == I_knob_MEMORY ) {
		// Memory throttling in the extended context
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
		if ( operation == reading_operation ) {
			irc = PWR_ObjAttrGetValue (p_obj_ext[I_pext_MEM0+icmg], PWR_ATTR_THROTTLING_STATE, &u64, NULL);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GetValue", p_ext_name[I_pext_MEM0+icmg]); return(irc); }
			value = u64;
		} else {
			if ( !(0 <= value && value <= 9) ) {
				warning_print ("SetValue", p_ext_name[I_pext_MEM0+icmg], "invalid throttling", value); return(-1);
				}
			u64 = value;
			irc = PWR_ObjAttrSetValue (p_obj_ext[I_pext_MEM0+icmg], PWR_ATTR_THROTTLING_STATE, &u64);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "SetValue", p_ext_name[I_pext_MEM0+icmg]); return(irc); }
		}
		}

	} else
	if ( knob == I_knob_ISSUE ) {
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			p_obj_grp = NULL;
			irc = PWR_ObjGetChildren (p_obj_ext[I_pext_CMG0CORES+icmg], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GetChildren", p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }
		if ( operation == reading_operation ) {
			irc = PWR_GrpAttrGetValue (p_obj_grp, PWR_ATTR_ISSUE_STATE, u64array, NULL, NULL);
			(void) PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GrpGet (ISSUE)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }
			value = (int)u64array[0];
		} else {
			irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_ISSUE_STATE, u64array, NULL);
			(void) PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GrpSet (ISSUE)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }
		}
		}

	} else
	if ( knob == I_knob_PIPE ) {
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			p_obj_grp = NULL;
			irc = PWR_ObjGetChildren (p_obj_ext[I_pext_CMG0CORES+icmg], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GetChildren", p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }

		if ( operation == reading_operation ) {
			irc = PWR_GrpAttrGetValue (p_obj_grp, PWR_ATTR_EX_PIPE_STATE, u64array, NULL, NULL);
			(void) PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GrpGet (PIPE)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }
			value = (int)u64array[0];
		} else {
			irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_EX_PIPE_STATE, u64array, NULL);
			(void) PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GrpSet (PIPE)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }
		}
		}

	} else
	if ( knob == I_knob_ECO ) {
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			p_obj_grp = NULL;
			irc = PWR_ObjGetChildren (p_obj_ext[I_pext_CMG0CORES+icmg], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GetChildren", p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }

		if ( operation == reading_operation ) {
			irc = PWR_GrpAttrGetValue (p_obj_grp, PWR_ATTR_ECO_STATE, u64array, NULL, NULL);
			irc = PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GrpGet (ECO)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }
			value = (int)u64array[0];
		} else {
			irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_ECO_STATE, u64array, NULL);
			irc = PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GrpSet (ECO)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(irc); }
		}
		}

	/**
	// RETENTION is disabled
	} else
	if ( knob == I_knob_RETENTION ) {
		warning_print ("my_power_bind_knobs", "RETENTION", "user RETENTION control is not allowed");
		return(-1);
	**/
	} else {
		//	should not reach here
		error_print(knob, "my_power_bind_knobs", "internal error. knob="); return(-1);
		;
	}

	return(0);
}


int my_power_bind_start (uint64_t pa64timer[], double w_joule[])
{
	int irc;

	if ( (Max_power_object + Max_power_extended) == 0 ) {
		return 0;
	}
	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "<my_power_bind_start> Max_power_object=%d, Max_power_extended=%d\n", Max_power_object, Max_power_extended);
	#endif

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "\t starting objects in default context\n");
	#endif
	for (int i=0; i<Max_power_object; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_array[i], PWR_ATTR_ENERGY, &w_joule[i], &pa64timer[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("my_power_bind_start", p_obj_name[i], "default GetValue"); }
	}

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "\t starting objects in extended context\n");
	#endif
	int iadd=Max_power_object;
	//	for (int i=0; i<Max_power_extended; i++) {
	for (int i=0; i<Max_measure_device; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_ext[i], PWR_ATTR_MEASURED_ENERGY, &w_joule[i+iadd], &pa64timer[i+iadd]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("my_power_bind_start", p_ext_name[i], "extended GetValue"); }
	}

	return 0;
}


int my_power_bind_stop (uint64_t pa64timer[], double w_joule[])
{
	int irc;

	if ( (Max_power_object + Max_power_extended) == 0 ) {
		return 0;
	}
	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "\t <my_power_bind_stop> Max_power_object=%d, Max_power_extended=%d\n", Max_power_object, Max_power_extended);
	#endif

	for (int i=0; i<Max_power_object; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_array[i], PWR_ATTR_ENERGY, &w_joule[i], &pa64timer[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("my_power_bind_stop", p_obj_name[i], "default GetValue"); }
	}

	int iadd=Max_power_object;
	//	for (int i=0; i<Max_power_extended; i++) {
	for (int i=0; i<Max_measure_device; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_ext[i], PWR_ATTR_MEASURED_ENERGY, &w_joule[i+iadd], &pa64timer[i+iadd]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("my_power_bind_stop", p_ext_name[i], "extended GetValue"); }
	}

	return 0;
}


int my_power_bind_finalize ()
{
	int irc;

	irc = 0;
	irc = PWR_CntxtDestroy(pacntxt);
	irc += PWR_CntxtDestroy(extcntxt);

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr, "\t <my_power_bind_finalize> returns %d\n", irc);
	#endif

	return irc;
}


void error_print(int irc, std::string cstr1, std::string cstr2)
{
	fprintf(stderr, "*** PMlib Error. <power_ext::%s> failed. [%s] return code %d \n", cstr1.c_str(), cstr2.c_str(), irc);
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3)
{
	fprintf(stderr, "*** PMlib Warning. <power_ext::%s> failed. [%s] %s \n", cstr1.c_str(), cstr2.c_str(), cstr3.c_str());
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3, int value)
{
	fprintf(stderr, "*** PMlib Warning. <power_ext::%s> failed. [%s] %s : value %d \n", cstr1.c_str(), cstr2.c_str(), cstr3.c_str(), value);
	return;
}

