#undef _OPENMP

#include <mpi.h>
#include <stdio.h>
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <PerfMonitor.h>
using namespace pm_lib;
extern void stream();
extern void timer_summary();

const int Max_power_knob=5;

PerfMonitor PM;
int my_id, npes, num_threads;
int iknob, value;

int main (int argc, char *argv[])
{

double flop_count;
double tt, t1, t2, t3, t4;
int i, j, loop, num_threads, iret;
float real_time, proc_time, mflops;
long long flpops;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);
#ifdef _OPENMP
	num_threads  = omp_get_max_threads();
#else
	num_threads  = 1;
#endif
	if(my_id == 0) fprintf(stderr, "<main> starting STREAM test\n");


	PM.initialize();

/* If you want to play with power knobs, here you go...
 *
 *
 */

	for (int i=0; i<Max_power_knob ; i++)
	{
		iknob=i;
		PM.getPowerKnob(iknob, value);
		if(my_id == 0) fprintf(stderr, "preset value of power knob %d is %d \n", iknob, value);
	}

	for (int i=0; i<Max_power_knob ; i++)
	{
		if(my_id == 0) fprintf(stderr, "set the new value of power knob %d is %d \n", iknob, value);
		iknob=i;
		value=1;
		if(i == 0) value=2200;
		if(i == 1) value=5;
		PM.setPowerKnob(iknob, value);
	}

	PM.start("slow_stream");
	//	PM.start("stream_check");
	t1=MPI_Wtime();
	stream();
	t2=MPI_Wtime();
	MPI_Barrier(MPI_COMM_WORLD);
	//	PM.stop ("stream_check");
	PM.stop ("slow_stream");
	if(my_id == 0) fprintf(stderr, "STREAM finished in %f seconds\n", t2-t1);
	if(my_id == 0) (void) timer_summary();

	PM.report(stdout);

	MPI_Finalize();
	if(my_id == 0) fprintf(stderr, "<main> finished\n");

return 0;
}

