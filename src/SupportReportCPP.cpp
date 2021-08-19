#include <stdio.h>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <PerfMonitor.h>

extern pm_lib::PerfMonitor PM;
#ifdef _OPENMP
	#if defined (__INTEL_COMPILER)  || \
	defined (__GXX_ABI_VERSION) || \
	defined (__CLANG_FUJITSU)   || \
	defined (__PGI)
	// the measurement inside and/or outside of parallel region is supported
	#pragma omp threadprivate(PM)

	#else
	// Only the  measurement outside of parallel region is supported
	#endif
#endif



namespace pm_lib {

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
  void PerfReport::report(FILE* fp)
  //	void merge_and_report(FILE* fp)
  {
	#ifdef DEBUG_PRINT_MONITOR
    if (PM.my_rank==0) fprintf(stderr, "\n<PerfMonitor::report> start \n");
	#endif

	int id, mid, inside;
	int nSections;

//	stop the Root section before report

	id = 0;
	PM.SerialParallelRegion (id, mid, inside);

	if (inside==0) {
		PM.stopRoot ();
	} else if (inside==1) {
		#ifdef _OPENMP
		#pragma omp parallel
		#endif
		{

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
		#ifdef DEBUG_PRINT_MONITOR
		printf("section %d is %s \n", id, (inside==0)?"outside":"inside");
		#endif

	if (inside==0) {
		// The section is defined outside of parallel context
		PM.mergeThreads (id);

	} else if (inside==1) {
		// The section is defined inside parallel context
		// If an OpenMP parallel region is started by a Fortran routine,
		// the merge operation must be triggered by a Fortran routine,
		// i.e. C or C++ parallel context does not match that of Fortran.
		// The followng OpenMP parallel block profives such merging support.
		#ifdef _OPENMP
		#pragma omp parallel
		#endif
		PM.mergeThreads (id);
	} else {
		;
	}
	}	// end of for loop

//	now start reporting the PMlib stats
	PM.selectReport (fp);
	return;
  }

} // end of namespace

