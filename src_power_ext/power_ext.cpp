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
#include "pmlib_power.h"

// ################################################################
// 2021/5/17 K. Mikami
// Testing Power APIs for measured and estimated components
static void error_print(int , std::string , std::string);
static void warning_print (std::string , std::string , std::string );
static void warning_print (std::string , std::string , std::string , int);
// ################################################################

/**

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

const int Max_measure_device=1;	// "plat.node" is the only attribute available to get the measured value
const int Max_power_leaf_parts=12;	// max. # of leaf parts in the object group, i.e. 12 cores in the CMG.

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

**/

PWR_Cntxt pacntxt = NULL;				// typedef void* PWR_Cntxt
PWR_Cntxt extcntxt = NULL;
PWR_Obj p_obj_array[Max_power_object];		// typedef void* PWR_Obj
PWR_Obj p_obj_ext[Max_power_extended];
uint64_t u64array[Max_power_leaf_parts];
uint64_t u64;


int my_power_bind_initialize (void)
{
	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr,"<my_power_initialize> objects in default context\n");
	#endif

	int irc;

	irc = PWR_CntxtInit (PWR_CNTXT_DEFAULT, PWR_ROLE_APP, "app", &pacntxt);
	if (irc != PWR_RET_SUCCESS) error_print(irc, "Power API PWR_CntxtInit", "default");

	for (int i=0; i<Max_power_object; i++) {
		irc = PWR_CntxtGetObjByName (pacntxt, p_obj_name[i], &p_obj_array[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("Power API Cntxt GetObj", p_obj_name[i], "default"); continue;}
	}

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr,"<my_power_initialize> objects in extended context\n");
	#endif

	irc = PWR_CntxtInit (PWR_CNTXT_FX1000, PWR_ROLE_APP, "app", &extcntxt);
	if (irc != PWR_RET_SUCCESS) error_print(irc, "Power API PWR_CntxtInit", "extended");

	for (int i=0; i<Max_power_extended; i++) {
		irc = PWR_CntxtGetObjByName (extcntxt, p_ext_name[i], &p_obj_ext[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("Power API Cntxt GetObj", p_ext_name[i], "extended"); continue;}
	}
	return(0);
}


int my_power_bind_knob ( int knob, int value)
{
	PWR_Grp p_obj_grp = NULL;
	int irc;


	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr,"<my_power_bind_knob> knob=%d, value=%d\n", knob, value);
	#endif

	//	value for I_knob_CPU    : cpu frequency (MHz) : 2200, 2000, (1600 is possible for limited use)
	//	value for I_knob_MEMORY : memory access throttling (%) : 100, 90, 80, .. , 10
	//	value for I_knob_ISSUE  : instruction issues (/cycle) : 2, 4
	//	value for I_knob_PIPE   : number of PIPEs : 1, 2
	//	value for I_knob_ECO    : eco state (mode) : 0, 1, 2
	//	value for I_knob_RETENTION : retention state (mode) : 0, 1 // retention is not allowed as of 2021 May

	if ( knob < 0 | knob >Max_power_knob ) {
		error_print(knob, "my_power_bind_knob", "invalid controler");
		return(-1);
	}

	if ( knob == I_knob_CPU ) {
		double Hz;
		if ( !(value == 2200 || value == 2000) ) {
			//	1.6 GHz retention frequency can not be set by users
			warning_print ("Power API AttrSetValue", p_obj_name[I_pobj_CPU], "invalid frequency value", value);
			return(-1);
		}
		Hz = (double)value * 1.0e6;
		irc = PWR_ObjAttrSetValue (p_obj_array[I_pobj_CPU], PWR_ATTR_FREQ, &Hz);
		if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API AttrSetValue", p_obj_name[I_pobj_CPU]); return(-1); }
	} else
	if ( knob == I_knob_MEMORY ) {
		if ( value < 0 || value > 10 ) {
			warning_print ("Power API AttrSetValue", p_ext_name[I_knob_MEMORY], "invalid value", value);
			return(-1);
		}
		u64 = value;
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			#ifdef DEBUG_PRINT_POWER_EXT
			fprintf(stderr,"\t setting CMG%d [%s] memory bandwidth : %d\n", icmg, p_ext_name[I_pext_MEM0+icmg], value);
			#endif
			irc = PWR_ObjAttrSetValue (p_obj_ext[I_pext_MEM0+icmg], PWR_ATTR_THROTTLING_STATE, &u64);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API AttrSetValue", p_ext_name[I_pext_MEM0+icmg]); return(-1); }
		}

	} else
	if ( knob == I_knob_ISSUE ) {
		for (int i=0; i < Max_power_leaf_parts; i++) {
			u64array[i] = value;
		}
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			p_obj_grp = NULL;
			irc = PWR_ObjGetChildren (p_obj_ext[I_pext_CMG0CORES+icmg], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API GetChildren", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			#ifdef DEBUG_PRINT_POWER_EXT
			fprintf(stderr,"\t setting CMG%d [%s] ISSUE value : %d \n", icmg, p_ext_name[I_pext_CMG0CORES+icmg], value);
			#endif

			irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_ISSUE_STATE, u64array, NULL);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API GrpSet(ISSUE)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			irc = PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API PWR_GrpDestroy", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
		}

	} else
	if ( knob == I_knob_PIPE ) {
		for (int i=0; i < Max_power_leaf_parts; i++) {
			u64array[i] = value;
		}
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			p_obj_grp = NULL;
			irc = PWR_ObjGetChildren (p_obj_ext[I_pext_CMG0CORES+icmg], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API GetChildren", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			#ifdef DEBUG_PRINT_POWER_EXT
			fprintf(stderr,"\t setting CMG%d [%s] execution PIPE : %d \n", icmg, p_ext_name[I_pext_CMG0CORES+icmg], value);
			#endif

			irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_EX_PIPE_STATE, u64array, NULL);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API GrpSet(PIPE)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			irc = PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API PWR_GrpDestroy", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
		}

	} else
	if ( knob == I_knob_ECO ) {
		for (int i=0; i < Max_power_leaf_parts; i++) {
			u64array[i] = value;
		}
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			p_obj_grp = NULL;
			irc = PWR_ObjGetChildren (p_obj_ext[I_pext_CMG0CORES+icmg], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API GetChildren", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			#ifdef DEBUG_PRINT_POWER_EXT
			fprintf(stderr,"\t setting CMG%d [%s] ECO state : %d \n", icmg, p_ext_name[I_pext_CMG0CORES+icmg], value);
			#endif

			irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_ECO_STATE, u64array, NULL);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API GrpSet(ECO)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			irc = PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API PWR_GrpDestroy", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
		}

	} else
	if ( knob == I_knob_RETENTION ) {
		error_print(-1, "Power API",  "user RETENTION control is not allowed on Fugaku");
		return(-1);

		for (int i=0; i < Max_power_leaf_parts; i++) {
			u64array[i] = value;
		}
		for (int icmg=0; icmg <4 ; icmg++)	//	from I_pobj_CMG0CORES to I_pobj_CMG3CORES
		{
			p_obj_grp = NULL;
			irc = PWR_ObjGetChildren (p_obj_ext[I_pext_CMG0CORES+icmg], &p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "GetChildren", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			#ifdef DEBUG_PRINT_POWER_EXT
			fprintf(stderr,"\t setting CMG%d [%s] retention state : %d \n", icmg, p_ext_name[I_pext_CMG0CORES+icmg], value);
			#endif

			irc = PWR_GrpAttrSetValue (p_obj_grp, PWR_ATTR_RETENTION_STATE, u64array, NULL);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API GrpSet(RETENTION)",  p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
			irc = PWR_GrpDestroy (p_obj_grp);
			if (irc != PWR_RET_SUCCESS) { error_print(irc, "Power API PWR_GrpDestroy", p_ext_name[I_pext_CMG0CORES+icmg]); return(-1); }
		}
	} else {
		//	should not reach here
		error_print(knob, "my_power_bind_knob", "internal error. knob="); return(-1);
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
	fprintf(stderr,"<my_power_bind_start> Max_power_object=%d, Max_power_extended=%d\n", Max_power_object, Max_power_extended);
	#endif

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr,"\t starting objects in default context\n");
	#endif
	for (int i=0; i<Max_power_object; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_array[i], PWR_ATTR_ENERGY, &w_joule[i], &pa64timer[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("Power API AttrGetValue", p_obj_name[i], "default attribute"); }
	}

	#ifdef DEBUG_PRINT_POWER_EXT
	fprintf(stderr,"\t starting objects in extended context\n");
	#endif
	int iadd=Max_power_object;
	//	for (int i=0; i<Max_power_extended; i++) {
	for (int i=0; i<Max_measure_device; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_ext[i], PWR_ATTR_MEASURED_ENERGY, &w_joule[i+iadd], &pa64timer[i+iadd]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("Power API AttrGetValue ", p_ext_name[i], "extended attribute"); }
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
	fprintf(stderr,"\t <my_power_bind_stop> Max_power_object=%d, Max_power_extended=%d\n", Max_power_object, Max_power_extended);
	#endif

	for (int i=0; i<Max_power_object; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_array[i], PWR_ATTR_ENERGY, &w_joule[i], &pa64timer[i]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("Power API AttrGetValue", p_obj_name[i], "default attribute"); }
	}

	int iadd=Max_power_object;
	//	for (int i=0; i<Max_power_extended; i++) {
	for (int i=0; i<Max_measure_device; i++) {
		irc = PWR_ObjAttrGetValue (p_obj_ext[i], PWR_ATTR_MEASURED_ENERGY, &w_joule[i+iadd], &pa64timer[i+iadd]);
		if (irc != PWR_RET_SUCCESS) { warning_print ("Power API AttrGetValue", p_ext_name[i], "extended attribute"); }
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
	fprintf(stderr,"\t <my_power_bind_finalize> returns %d\n", irc);
	#endif

	return irc;
}


void error_print(int irc, std::string cstr1, std::string cstr2)
{
	printf("*** Error. <%s> failed. [%s] return code %d \n", cstr1.c_str(), cstr2.c_str(), irc);
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3)
{
	printf("*** Warning. <%s> failed. [%s] %s \n", cstr1.c_str(), cstr2.c_str(), cstr3.c_str());
	return;
}
void warning_print (std::string cstr1, std::string cstr2, std::string cstr3, int value)
{
	printf("*** Warning. <%s> failed. [%s] %s : value %d \n", cstr1.c_str(), cstr2.c_str(), cstr3.c_str(), value);
	return;
}

