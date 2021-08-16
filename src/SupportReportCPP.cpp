
#ifdef DISABLE_MPI
#include "mpi_stubs.h"
#else
#include <mpi.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <string>
#include "stdlib.h"
#include <unistd.h>
#include <PerfMonitor.h>


	extern pm_lib::PerfMonitor PM;
	#pragma omp threadprivate(PM)


  //> PMlib report controll routine
  ///	@brief
  /// - [1] stop the Root section
  /// - [2] merge thread serial/parallel sections
  /// - [3] select the type of the report and start producing the report
  ///
  /// @param[in] FILE* fc     output file pointer
  ///
  ///   @note Most likely, this routine is called as report(stdout);
  ///
  void merge_and_report(FILE* fp)
  {
	#ifdef DEBUG_PRINT_MONITOR
    if (my_rank==0) fprintf(stderr, "\n<PerfMonitor::report> start \n");
	#endif

	int id, mid, inside;
	int nSections;

//	stop the Root section before report

	id = 0;
	PM.SerialParallelRegion (id, mid, inside);

	if (inside==0) {
		PM.stopRoot ();
	} else if (inside==1) {
		#pragma omp parallel
		{
		#ifdef DEBUG_PRINT_MONITOR
		int i_th = omp_get_thread_num();
		fprintf(stderr, "<report> calling <stopRoot> my_thread=%d i_th=%d \n", my_thread, i_th);
		#endif

		PM.stopRoot ();
		}
	} else {
		;
	}

//	count the number of SHARED sections

	PM.countSections (nSections);

//	merge thread data into the master thread 

	for (id=0; id<nSections; id++) {
	PM.SerialParallelRegion (id, mid, inside);

	if (inside==0) {
		// The section is defined outside of parallel context
		PM.mergeThreads (id);

	} else if (inside==1) {
		// The section is defined inside parallel context
		// If an OpenMP parallel region is started by a Fortran routine,
		// the merge operation must be triggered by a Fortran routine,
		// i.e. C or C++ parallel context does not match that of Fortran.
		// The followng OpenMP parallel block profives such merging support.
		#pragma omp parallel
		PM.mergeThreads (id);
	} else {
		;
	}
	}	// end of for loop

//	now start reporting the PMlib stats
	PM.selectReport (fp);
	return;
  }

