#undef _OPENMP
// pmlib C++ test program based on stream.c by John McCalpin
#define FLT_MAX 1.0e+16

#ifdef _OPENMP
#include <omp.h>
#endif
#include <PerfMonitor.h>
using namespace pm_lib;
extern PerfMonitor PM;

# define N	20000000
//	# define N	10000000
//	# define NTIMES	1000
//	# define NTIMES	100
# define NTIMES	100
# define OFFSET	0

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

static double	a[N+OFFSET], b[N+OFFSET], c[N+OFFSET];
static double	avgtime[4] = {0}, maxtime[4] = {0},
		mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};
static char	*label[4] = {"Copy:      ", "Scale:     ",
    "Add:       ", "Triad:     "};

static double	bytes[4] = {
    2 * sizeof(double) * N,
    2 * sizeof(double) * N,
    3 * sizeof(double) * N,
    3 * sizeof(double) * N
    };

extern double mysecond();

void stream()
{
    int			quantum, checktick();
    int			BytesPerWord;
    register int	j, k;
    double		scalar, times[4][NTIMES];

    printf("Modified STREAM Array size = %d\n" , N);

#ifdef _OPENMP
#pragma omp parallel 
    {
#pragma omp master
	{
	    k = omp_get_num_threads();
	    printf ("Number of Threads requested = %i\n",k);
        }
    }
#endif


    /* Get initial value for system clock. */
#pragma omp parallel for
    for (j=0; j<N; j++) {
	a[j] = 1.0;
	b[j] = 2.0;
	c[j] = 0.0;
	}

    /*	--- MAIN LOOP --- repeat test cases NTIMES times --- */

    scalar = 3.0;
    for (k=0; k<NTIMES; k++)
	{
/*
 */
	PM.start("sub1_copy");
	times[0][k] = mysecond();
#pragma omp parallel for
	for (j=0; j<N; j++)
	    c[j] = a[j];
	times[0][k] = mysecond() - times[0][k];
	PM.stop ("sub1_copy");

	PM.start("sub2_scale");
	times[1][k] = mysecond();
#pragma omp parallel for
	for (j=0; j<N; j++)
	    b[j] = scalar*c[j];
	times[1][k] = mysecond() - times[1][k];
	PM.stop ("sub2_scale");

	PM.start("sub3_add");
	times[2][k] = mysecond();
#pragma omp parallel for
	for (j=0; j<N; j++)
	    c[j] = a[j]+b[j];
	times[2][k] = mysecond() - times[2][k];
	PM.stop ("sub3_add");
	
	PM.start("sub4_triad");
	times[3][k] = mysecond();
#pragma omp parallel for
	for (j=0; j<N; j++)
	    a[j] = b[j]+scalar*c[j];
	times[3][k] = mysecond() - times[3][k];
	PM.stop ("sub4_triad");
/*
 */
	}

    /*	--- SUMMARY --- */

    for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{
	for (j=0; j<4; j++)
	    {
	    avgtime[j] = avgtime[j] + times[j][k];
	    mintime[j] = MIN(mintime[j], times[j][k]);
	    maxtime[j] = MAX(maxtime[j], times[j][k]);
	    }
	}
    
    printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
    for (j=0; j<4; j++) {
	avgtime[j] = avgtime[j]/(double)(NTIMES-1);

	printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
	       1.0E-06 * bytes[j]/mintime[j],
	       avgtime[j],
	       mintime[j],
	       maxtime[j]);
    }
	//	for (j=0; j<N; j++) {
	//	if ((j % 100000) == 0 ) printf("%f\n", a[j]);
	//	}
}

# define	M	20

int
checktick()
    {
    int		i, minDelta, Delta;
    double	t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */

    for (i = 0; i < M; i++) {
	t1 = mysecond();
	while( ((t2=mysecond()) - t1) < 1.0E-6 )
	    ;
	timesfound[i] = t1 = t2;
	}

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

    minDelta = 1000000;
    for (i = 1; i < M; i++) {
	Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
	minDelta = MIN(minDelta, MAX(Delta,0));
	}

   return(minDelta);
}


/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

