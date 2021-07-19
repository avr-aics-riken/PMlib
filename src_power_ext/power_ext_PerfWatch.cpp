
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
#include "power_obj_menu.h"

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
