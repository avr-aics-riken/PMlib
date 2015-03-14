/* ##################################################################
 *
 * PMlib - Performance Monitor library
 *
 * Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
 * All rights reserved.
 *
 * Copyright (c) 2012-2015 Advanced Institute for Computational Science, RIKEN.
 * All rights reserved.
 *
 * ###################################################################
 */

//@file   papi_ext.c
//@brief  Interface for papi

#include "stdio.h"
#include "papi.h"
#include <string.h>

#define HL_STOP		0
#define HL_START	1

typedef struct _HighLevelInfo
{
	int EventSet;					        /**< EventSet of the thread */
	short int num_evts;				    /**< number of events in the eventset */
	short int running;				    /**< STOP, START, or RATE */
	long long initial_real_time;	/**< Start real time */
	long long initial_proc_time;	/**< Start processor time */
	long long last_real_time;		  /**< Previous value of real time */
	long long last_proc_time;		  /**< Previous value of processor time */
	long long total_ins;			    /**< Total instructions */
} HighLevelInfo;

extern void _internal_cleanup_hl_info( HighLevelInfo * state );
extern int _internal_check_state( HighLevelInfo ** state );
extern int _internal_start_hl_counters( HighLevelInfo * state );
extern int _internal_hl_read_cnts( long long *values, int array_len, int flag );

// Additional C function to link with C++ pmlib class


void print_state_HighLevelInfo(HighLevelInfo *state)
{
	if ( state == NULL ) {
	// internally, the program should not reach this statement
		fprintf(stderr,"*** error. <print_state_HighLevelInfo> state==NULL\n");
		return;
	}

	#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr,"\t <print_state_HighLevelInfo> starts\n");
	fprintf(stderr,"\t struct state:  EventSet=%d, num_evts=%d, running=%d\n",
						state->EventSet, state->num_evts, state->running);
	#endif
}


int my_papi_add_events ( int *events, int num_events)
{
	HighLevelInfo *state = NULL;
	int i, retval;

	#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr,"\t <my_papi_add_events> starts. num_events=%d\n", num_events);
	for (i=0; i<num_events; i++){
		fprintf(stderr, "  i:%d events[i]:%u\n", i,events[i]);
	}
	#endif

	if ( num_events == 0 ) {
		fprintf(stderr,"*** info <my_papi_add_events> No event\n");
		return PAPI_OK;
	}

	if ( ( retval = _internal_check_state( &state ) ) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_add_events> :: <_state>\n");
		return retval;
	}

	if (( retval = PAPI_add_events( state->EventSet, events, num_events )) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_add_events> :: <PAPI_add_events> state->EventSet=%d, num_events=%d\n", state->EventSet, num_events);
		_internal_cleanup_hl_info( state );
		PAPI_cleanup_eventset( state->EventSet );
		return retval;
	}
	print_state_HighLevelInfo(state);

	return PAPI_OK;
}


int my_papi_bind_start ( int *events, long long *values, int num_events)
{
	HighLevelInfo *state = NULL;
	int retval;
	#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr,"\t <my_papi_bind_start> starts\n");
	#endif

	if ( num_events == 0 ) {
		//	fprintf(stderr,"*** info <my_papi_bind_start> No event\n");
		return PAPI_OK;
	}

	if ( ( retval = _internal_check_state( &state ) ) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_bind_start> :: <_state>\n");
		return retval;
	}
	if ( ( retval = PAPI_start( state->EventSet ) ) != PAPI_OK ) {
		if ( retval == PAPI_EISRUN) {
		// Event set is already running.
		// This is usually the case for multi level measurement sections
		// As of current version, we ignore this, i.e. single level support
		}
		else {
			fprintf(stderr,"*** error. <my_papi_bind_start> :: <PAPI_start>\n");
			return retval;
		}
	}
	state->running = HL_START;
	#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr,"\t <my_papi_bind_start> returns\n");
	#endif

	return PAPI_OK;
}


int my_papi_bind_stop ( int *events, long long *values, int num_events)
{
	HighLevelInfo *state = NULL;
	int retval;
	#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr,"\t <my_papi_bind_stop> starts\n");
	#endif

	if ( num_events == 0 ) {
		//	fprintf(stderr,"*** info <my_papi_bind_stop> No event\n");
		return PAPI_OK;
	}

	if ( ( retval = _internal_check_state( &state ) ) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_bind_stop> :: <_state>\n");
		return retval;
	}
	if ( ( retval = PAPI_stop( state->EventSet, values ) ) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_bind_stop> :: <PAPI_stop>\n");
		state->running = HL_STOP;
		return retval;
	}
	if ( ( retval = PAPI_start( state->EventSet ) ) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_bind_stop> :: <PAPI_start>\n");
		state->running = HL_STOP;
		return retval;
	}
	return PAPI_OK;
}


int my_papi_bind_read ( int *events, long long *values, int num_events)
{
	HighLevelInfo *state = NULL;
	int retval;
	#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr,"\t <my_papi_bind_read> starts\n");
	#endif

	if ( num_events == 0 ) {
		//	fprintf(stderr,"*** info <my_papi_bind_read> No event\n");
		return PAPI_OK;
	}

	if ( ( retval = _internal_check_state( &state ) ) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_bind_read> :: <_state> \n");
		return retval;
	}
	if ( ( retval = PAPI_read( state->EventSet, values ) ) != PAPI_OK ) {
		fprintf(stderr,"*** error. <my_papi_bind_read> :: <PAPI_read>\n");
		return retval;
	}
	return PAPI_OK;
}


void my_papi_name_to_code ( char* c_event, int* i_event)
{
	int retval;
	//	retval = PAPI_event_name_to_code( c_event, &i_event );

	retval = PAPI_event_name_to_code( c_event, i_event );
	if ( retval != PAPI_OK ) {
		fprintf(stderr,"*** error. <PAPI_event_name_to_code> c_event=[%s], retval=%d\n", c_event, retval);
		return;
	}
	#ifdef DEBUG_PRINT_PAPI
	fprintf(stderr,"\t <my_papi_name_to_code> c_event=[%s], i_event=%u\n",
		c_event, *i_event);
	#endif

	return;
}

