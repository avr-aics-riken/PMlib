//
// Prototype definitions for C program
//
extern void C_pm_initialize (int init_nWatch);
extern void C_pm_start (char* fc);
extern void C_pm_stop (char* fc);
extern void C_pm_stop_usermode (char* fc, double fpt, unsigned tic);
extern void C_pm_report (char* fc);
extern void C_pm_select_report (char* fc);
extern void C_pm_print (char* fc, char* fh, char* fcmt, int fp_sort);
extern void C_pm_printdetail (char* fc, int legend, int fp_sort);
extern void C_pm_printthreads (char* fc, int rank_ID, int fp_sort);
extern void C_pm_printlegend (char* fc);
extern void C_pm_printprogress (char* fc, char* comments, int fp_sort);
extern void C_pm_posttrace (void);
extern void C_pm_reset (char* fc);
extern void C_pm_resetall (void);
extern void C_pm_setproperties (char* fc, int f_type, int f_exclusive);
extern void C_pm_gather (void);
extern void C_pm_sections (int *nSections);
extern void C_pm_serial_parallel (int id, int *mid, int *inside);
extern void C_pm_stop_Root (void);
extern void C_pm_mergethreads (int id);
extern void C_pm_getpowerknob (int knob, int* value);
extern void C_pm_setpowerknob (int knob, int value);
#if defined  (MORE_MPI_MEMBERS)
extern void C_pm_printgroup (char* fc, MPI_Group p_group, MPI_Comm p_comm, int* pp_ranks, int group, int legend, int fp_sort);
extern void C_pm_printcomm (char* fc, MPI_Comm new_comm, int icolor, int key, int legend, int fp_sort);
#endif
