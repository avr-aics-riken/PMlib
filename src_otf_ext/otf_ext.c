#ifdef USE_OTF

#include "stdio.h"
#include <string.h>
#include "otf.h"
#define OTF_STREAM_0 0

	static OTF_FileManager* manager;
	static OTF_WStream* wstream;
	static OTF_Writer* writer;
	static OTF_MasterControl* master;
	static double otf_base_time;
	static uint32_t otf_counterid= 5;
	static uint32_t otf_counterGroup= 63;

// num_process	並列プロセス数
// my_rank	自ランク番号
// m_type	測定対象タイプ (0:通信, 1:計算)
// otf_filename	OTF filename header 文字列
// baseT	base time to start recording from.

void my_otf_initialize(int num_process, int my_rank, char* otf_filename, double baseT)
{
	// Initialize OTF output files
	// this function is called by PerfMonitor::initialize (int init_nWatch)
	// remark OTF process id is numbered as 1,2,..,num_process, not from 0.

	#ifdef DEBUG_PRINT_OTF
	if (my_rank == 0) {
	fprintf(stderr, "\t<my_otf_initialize> num_process=%d, my_rank=%d, filename=%s, baseT=%f \n", num_process, my_rank, otf_filename, baseT);
	}
	#endif

	manager = OTF_FileManager_open(num_process);
	if (!manager) {
		fprintf(stderr, "\t*** internal error. <my_otf_initialize> OTF_FileManager_open() failed. \n");
		return;
	}

	//	if (my_rank == 0) {
	writer = OTF_Writer_open(otf_filename, num_process, manager);
	if (!writer) {
		fprintf(stderr, "\t*** internal error. <my_otf_initialize> OTF_Writer_open() failed. \n");
		return;
	}
	//	}

	wstream = OTF_WStream_open(otf_filename, my_rank+1, manager);
	if (!wstream) {
		fprintf(stderr, "\t*** internal error. <my_otf_initialize> OTF_WStream_open() failed. \n");
		return;
	}

	otf_base_time = baseT;

}


// my_rank	自ランク番号
// time		the event start time
// m_id		the number mapped to the section label (1,2,..,m_nWatch)

void my_otf_event_start(int my_rank, double time, int m_id)
{
	// Write the counter zero value and leave with starting time stamp
	// this function is called by PerfWatch::start()
	// m_id is the sequential number mapped with the section label
	// remark OTF identifier is numbered as 1,2,..,num_process. 0 is reserved.
	// so the numbers given from PMlib are +1 biased.

	uint64_t m_sec = (uint64_t)((time-otf_base_time)*1.0e6);
	uint64_t u_w = (uint64_t)0;
	OTF_WStream_writeEnter(wstream, m_sec, m_id+1, my_rank+1, (uint32_t)0);
	OTF_WStream_writeCounter (wstream, m_sec, my_rank+1, otf_counterid, u_w);

	#ifdef DEBUG_PRINT_OTF
	fprintf(stderr, "\t<my_otf_event_start> my_rank=%d, time=%f, m_sec=%lu, m_id=%d\n", my_rank, time, m_sec, m_id);
	#endif
}



// my_rank	自ランク番号
// time		the event start time
// m_id		the number mapped to the section label (1,2,..,m_nWatch)
// m_type	測定対象タイプ (0:通信, 1:計算)
// w		測定値 (ユーザ指定値 or HWPC自動測定値)

void my_otf_event_stop(int my_rank, double time, int m_id, int m_type, double w)
{
	// Write the counter value and leave with ending time stamp
	// this function is called by PerfWatch::stop()
	uint64_t m_sec = (uint64_t)((time-otf_base_time)*1.0e6);
	uint64_t u_w = (uint64_t)w;

	OTF_WStream_writeCounter (wstream, m_sec, my_rank+1, otf_counterid, u_w);
	OTF_WStream_writeLeave(wstream, m_sec, m_id+1, my_rank+1, (uint32_t)0);

	#ifdef DEBUG_PRINT_OTF
	fprintf(stderr, "\t<my_otf_event_stop> my_rank=%d, time=%f, m_id=%d, m_sec=%lu, w=%e, u_w=%lu \n", my_rank, time, m_id, m_sec, w, u_w);
	#endif
}


// num_process	並列プロセス数
// my_rank	自ランク番号
// id		the number mapped to the section label (1,2,..,m_nWatch)
// c_label	測定区間のラベル文字列

void my_otf_event_label(int num_process, int my_rank, int id, char* c_label)
{
	// create OTF *.def file
	if (my_rank != 0) return;

	#ifdef DEBUG_PRINT_OTF
	fprintf(stderr, "\t<my_otf_event_label> id=%d, c_label=%s\n", id, c_label);
	#endif

	if (id == 1) {
		char c_defp[21];
		OTF_Writer_writeDefCreator( writer, OTF_STREAM_0, "PMlib created OTF");

		OTF_Writer_writeDefTimerResolution(writer, OTF_STREAM_0, (uint64_t)1.0e6);
		//	The default timer resolution is 1 micro second (1e6 ticksPerSecond)
		//	writes "TR" record

		for( int i= 1; i <= num_process; i++ ) {
			snprintf( c_defp, 20, "Process %u", i );
			OTF_Writer_writeDefProcess (writer, OTF_STREAM_0, i, c_defp, 0);
		}
		OTF_Writer_writeDefFunctionGroup
			( writer, OTF_STREAM_0, 16, "Root Section");
		OTF_Writer_writeDefFunctionGroup
			( writer, OTF_STREAM_0, 17, "User Initialized Sections");
		OTF_Writer_writeDefFunction( writer, OTF_STREAM_0, id, c_label, 16, 0);

	} else {
		OTF_Writer_writeDefFunction( writer, OTF_STREAM_0, id, c_label, 17, 0);
	}

	#ifdef DEBUG_PRINT_OTF
	fprintf(stderr, "\t<my_otf_event_label> my_rank=%d returns.\n", my_rank);
	#endif
}


// num_process	並列プロセス数
// my_rank	自ランク番号
// otf_filename	OTF filename header 文字列
// c_group		counter group name 文字列
// c_counter	測定counter name 文字列
// c_unit		測定unit name 文字列

void my_otf_finalize(int num_process, int my_rank, char* otf_filename,
					char* c_group, char* c_counter, char* c_unit)
{
	#ifdef DEBUG_PRINT_OTF
	if (my_rank == 0) {
	fprintf(stderr, "\t<my_otf_finalize> num_process=%d, my_rank=%d, otf_filename=%s \n", num_process, my_rank, otf_filename);
	}
	#endif

	if (my_rank == 0) {
	OTF_Writer_writeDefCounterGroup( writer, OTF_STREAM_0,
        otf_counterGroup, c_group );

	OTF_Writer_writeDefCounter( writer, OTF_STREAM_0,
        otf_counterid, c_counter,
        OTF_COUNTER_TYPE_ABS | OTF_COUNTER_SCOPE_LAST,
        otf_counterGroup, c_unit);
	}

	OTF_WStream_close(wstream);
	//	if (my_rank == 0) {
	OTF_Writer_close(writer);
	//	}

	if (my_rank == 0) {
		master = OTF_MasterControl_new(manager);
		if (master == NULL) {
			fprintf(stderr, "\t*** PMlib internal error. <my_otf_initialize> OTF_MasterControl_new() failed. \n");
			return;
		}

		for (int i=1; i <= num_process; i++) {
			OTF_MasterControl_append(master, i, i);
		}
		OTF_MasterControl_write(master, otf_filename);
		OTF_MasterControl_close(master);
	}
	OTF_FileManager_close(manager);
}
#endif


/*
Note on the arguments for OTF_Writer_writeDefCounter()

The combination of type and scope affects the result of visualized image
depending on the unit of GFlops, GB/s, etc. and on the visualization package.
The following combination is the best choice for Vampir.
	OTF_COUNTER_TYPE_ABS | OTF_COUNTER_SCOPE_LAST,

Other combinations can be:
	//	OTF_COUNTER_TYPE_ACC | OTF_COUNTER_SCOPE_START,
	//	OTF_COUNTER_TYPE_ACC | OTF_COUNTER_SCOPE_LAST,
	//	OTF_COUNTER_TYPE_ACC | OTF_COUNTER_SCOPE_POINT,
	//	OTF_COUNTER_TYPE_ABS | OTF_COUNTER_SCOPE_START,
	//	OTF_COUNTER_TYPE_ABS | OTF_COUNTER_SCOPE_LAST,
	//	OTF_COUNTER_TYPE_ABS | OTF_COUNTER_SCOPE_POINT,

type:
OTF_COUNTER_TYPE_ACC (default) a counter with monotonously increasing values
OTF_COUNTER_TYPE_ABS a counter with alternating absolute values

scope:
OTF_COUNTER_SCOPE_START (default) always refers to the start of the process,
OTF_COUNTER_SCOPE_POINT refers to exactly this moment in time,
OTF_COUNTER_SCOPE_LAST relates to the previous measurement, and
OTF_COUNTER_SCOPE_NEXT to the next measurement.

 */

