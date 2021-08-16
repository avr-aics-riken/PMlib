/// PMlib report controll routine for C program
/// - [1] stop the Root section
/// - [2] merge thread serial/parallel sections
/// - [3] select the type of the report and start producing the report
///
///	@param[in] char* fc		output file name
///
///	@note	if fc is NULL ("") , stdout is chosen.
///
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
//	#include "pmlib_api_C.h"

void C_pm_report (char *filename);
extern void C_pm_serial_parallel (int id, int *mid, int *inside);
extern void C_pm_stop_Root (void);
extern void C_pm_sections (int *nSections);
extern void C_pm_mergethreads (int id);
extern void C_pm_select_report (char *filename);

void C_pm_report (char *filename)
{
	int id, mid, inside;
	int nSections;

//	stop the Root section before report

	id = 0;
	C_pm_serial_parallel (id, &mid, &inside);

	if (inside==0) {
		C_pm_stop_Root ();
		;
	} else if (inside==1) {
		#pragma omp parallel
		C_pm_stop_Root ();
		;
	} else {
		;
	}

//	count the number of SHARED sections

	C_pm_sections (&nSections);

//	merge thread data into the master thread 

	for (id=0; id<nSections; id++) {
	C_pm_serial_parallel (id, &mid, &inside);

	if (inside==0) {
		// The section is defined outside of parallel context
		C_pm_mergethreads (id);

	} else if (inside==1) {
		// The section is defined inside parallel context
		// If an OpenMP parallel region is started by a Fortran routine,
		// the merge operation must be triggered by a Fortran routine,
		// i.e. C or C++ parallel context does not match that of Fortran.
		// The followng OpenMP parallel block profives such merging support.
		#pragma omp parallel
		C_pm_mergethreads (id);
		;
	} else {
		;
	}
	}	// end of for loop

//	now start reporting the PMlib stats
	C_pm_select_report (filename);
	return;
}
