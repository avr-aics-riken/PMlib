/*****************************************************************************
*  This example code shows how to use the newly coded highlevel interface
*  routines for PAPI
*  Those routines accept the specific events and report the statistics for them
******************************************************************************/ 

#include <stdio.h>
#include <stdlib.h>
#include "papi.h"

// following combination works on Intel Xeon E5-2670
//	#define NUM_EVENTS 7
//	int Events[NUM_EVENTS] = {PAPI_TOT_INS, PAPI_FP_INS, PAPI_FP_OPS, PAPI_SP_OPS, PAPI_DP_OPS, PAPI_LD_INS, PAPI_SR_INS};

// following combinations work on K
#define NUM_EVENTS 2
int Events[NUM_EVENTS] = {PAPI_LD_INS, PAPI_SR_INS};
//	int Events[NUM_EVENTS] = {PAPI_FP_INS, PAPI_FP_OPS};
//	int Events[NUM_EVENTS] = {PAPI_TOT_INS, PAPI_TOT_CYC};

// Additional macro patch for K computer which has obsolete PAPI version 3.6
// Recent Linux distro has PAPI version 5.2 or later,
#if defined(__FUJITSU) || defined(__HPC_ACE__)
#define PAPI_SP_OPS 0   // Needed for compatibility
#define PAPI_DP_OPS 0   // Needed for compatibility
#define PAPI_VEC_SP 0   // Needed for compatibility
#define PAPI_VEC_DP 0   // Needed for compatibility
#endif


#define THRESHOLD 10000
#define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__);  exit(retval); }

extern int my_papi_bind_start( int *, long long *, int );
extern int my_papi_bind_stop( int *, long long *, int );
extern int my_papi_bind_read( int *, long long *, int );


/* some kernel to be monitored, stupid codes as commented in PAPI source */ 
double computation_add()
{
   double tmp=1.0;
   double t1=1.0;
   double t2=2.0;
   for(int i = 1; i < THRESHOLD; i++ ) { tmp = tmp*t2 + t1; }
   return tmp;
}

void counter_printf(long long value[NUM_EVENTS], int n)
{
	fprintf(stderr, " n_events=%d, values[]: ", n);
	for(int i=0; i<n; i++)
		fprintf(stderr, " %llu,", value[i]);
		//	fprintf(stderr, " i=%d, value[i]=%llu \n", i, value[i]);
	fprintf(stderr, "\n");
}

int main()
{
   int n_Events=NUM_EVENTS;

   /*declaring place holder for no of hardware counters */
   int num_hwcntrs = 0;
   int retval;
   long long values[NUM_EVENTS];

   double x;

   if((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT )
   {
      fprintf(stderr, "*** error <PAPI_library_init> returned:%d\n",retval);
      fprintf(stderr, "header has PAPI_VER_CURRENT:%u\n",PAPI_VER_CURRENT);
      exit(1);
   }
      fprintf(stderr, "<PAPI_library_init> returned %d\n",retval);

   if ((num_hwcntrs = PAPI_num_counters()) < PAPI_OK) {
      printf("There are no counters available. \n"); exit(1);
   }
   printf("There can be maximum of  %d counters in this system\n",num_hwcntrs);

   printf("\nstart counters\n");
   if ( (retval = PAPI_start_counters(Events, NUM_EVENTS)) != PAPI_OK) {
	fprintf(stderr, "*** error <PAPI_start_counters> returned:%d\n",retval);
       ERROR_RETURN(retval);
   }
   printf(" Both <PAPI_read_counters> and <PAPI_accum_counters> reset the counter internally.\n");

//Section
   printf("\nDo computation %d times of (* & +) \n", THRESHOLD);
   x = computation_add();

   printf("<PAPI_read_counters> \n");
   retval=PAPI_read_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

   printf("<PAPI_read_counters> once more\n");
   retval=PAPI_read_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

//Section
   printf("\nDo computation %d times of (* & +) \n", THRESHOLD);
   x = computation_add();
 
   printf("<PAPI_accum_counters> \n");
   retval=PAPI_accum_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

   printf("<PAPI_accum_counters> once more\n");
   retval=PAPI_accum_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

//Section
   printf("\nDo computation %d times of (* & +) \n", THRESHOLD);
   x = computation_add();
 
   printf("<PAPI_read_counters> once more\n");
   retval=PAPI_read_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

   printf("<PAPI_accum_counters> \n");
   retval=PAPI_accum_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

//Section
   printf("\nDo computation %d times of (* & +) \n", THRESHOLD);
   x = computation_add();
 
   printf("<PAPI_accum_counters> \n");
   retval=PAPI_accum_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

   printf("<PAPI_read_counters> once more\n");
   retval=PAPI_read_counters(values, NUM_EVENTS); if (retval != PAPI_OK) exit(1);
   counter_printf(values,n_Events);

//Section
   printf("\n You see? \n");


//Section
   printf("\n\n");
   printf("Now we do low level counters...\n");
   printf("\nDo computation %d times of (* & +) \n", THRESHOLD);
   x = computation_add();
 
   retval=my_papi_bind_stop(Events, values, NUM_EVENTS);
		printf("my_papi_bind_stop retval=%d\n", retval);
   retval=my_papi_bind_start(Events, values, NUM_EVENTS);
		printf("my_papi_bind_start retval=%d\n", retval);

   printf("<my_papi_bind_read> \n");
   retval=my_papi_bind_read(Events, values, NUM_EVENTS);
	if (retval != PAPI_OK) { printf("*** Error retval=%d\n", retval); exit(1);}
   counter_printf(values,n_Events);

   printf("Plus computation %d times of (* & +) \n", THRESHOLD);
   x = computation_add();

   printf("<my_papi_bind_read> \n");
   retval=my_papi_bind_read(Events, values, NUM_EVENTS);
	if (retval != PAPI_OK) { printf("*** Error retval=%d\n", retval); exit(1);}
   counter_printf(values,n_Events);

//Section
   printf("\n");
   printf("<PAPI_stop_counters> \n");
   if ((retval=PAPI_stop_counters(values, NUM_EVENTS)) != PAPI_OK) {
      ERROR_RETURN(retval);	
   }
   counter_printf(values,n_Events);

   exit(0);	
}
