// pmlib C++ test program based on stream.c by John McCalpin
# include <stdio.h>
#ifdef _OPENMP
#include <omp.h>
#endif
//	#ifdef __cplusplus
//	extern "C" int omp_get_num_threads();
//	#else
//	extern int omp_get_num_threads();
//	#endif

# define FLT_MAX 1.0E+6
//	# define N	10000000
# define N	50000000
// if N >= 100M, the mcmodel compile option will be needed
# define NTIMES	10
# define OFFSET	0

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

static double	a[N+OFFSET], b[N+OFFSET], c[N+OFFSET];
static double	avgtime[4] = {0,0,0,0}, maxtime[4] = {0,0,0,0},
		mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};
static char	*label[4] = {"Copy:    ", "Scale:   ", "Add:     ", "Triad:   "};

static double	bytes[4] = {
    2 * sizeof(double) * (double)N,
    2 * sizeof(double) * (double)N,
    3 * sizeof(double) * (double)N,
    3 * sizeof(double) * (double)N
    };

extern double mysecond();

void stream_copy()
{
    int			quantum, checktick();
    int			BytesPerWord;
    register int	j, k;
    double		scalar, times[4][NTIMES];

    k = 0;
#ifdef _OPENMP
    k = omp_get_max_threads();
#endif
    printf("Modified STREAM COPY, num_threads=%d, array size= %d\n", k, N);

#pragma omp parallel for
    for (j=0; j<N; j++) {
	a[j] = 1.0;
	b[j] = 2.0;
	c[j] = 0.0;
	}

    scalar = 3.0;
    for (k=0; k<NTIMES; k++)
	{
	times[0][k] = mysecond();
#pragma omp parallel for
	for (j=0; j<N; j++)
	    c[j] = a[j];
	times[0][k] = mysecond() - times[0][k];

	}

    for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{
		j=0;
		avgtime[j] = avgtime[j] + times[j][k];
		mintime[j] = MIN(mintime[j], times[j][k]);
		maxtime[j] = MAX(maxtime[j], times[j][k]);
	}
    printf("Function    Rate (MB/s)   Avg time     Min time     Max time\n");
	{
    j=0;
	avgtime[j] = avgtime[j]/(double)(NTIMES-1);
	printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
	       1.0E-06 * bytes[j]/mintime[j], avgtime[j], mintime[j], maxtime[j]);
    }
}


void stream_triad()
{
    int			quantum, checktick();
    int			BytesPerWord;
    register int	j, k;
    double		scalar, times[4][NTIMES];

    k = 0;
#ifdef _OPENMP
    k = omp_get_max_threads();
#endif
    printf("Modified STREAM TRIAD, num_threads=%d, array size= %d\n", k, N);

#pragma omp parallel for
    for (j=0; j<N; j++) {
	a[j] = 1.0;
	b[j] = 2.0;
	c[j] = 0.0;
	}

    scalar = 3.0;
    for (k=0; k<NTIMES; k++)
	{
	times[3][k] = mysecond();
#pragma omp parallel for
	for (j=0; j<N; j++)
	    a[j] = b[j]+scalar*c[j];
	times[3][k] = mysecond() - times[3][k];
	}

    for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{
		j=3;
		avgtime[j] = avgtime[j] + times[j][k];
		mintime[j] = MIN(mintime[j], times[j][k]);
		maxtime[j] = MAX(maxtime[j], times[j][k]);
	}
    printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
	{
	j=3;
	avgtime[j] = avgtime[j]/(double)(NTIMES-1);
	printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
	       1.0E-06 * bytes[j]/mintime[j], avgtime[j], mintime[j], maxtime[j]);
    }
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

#ifdef __GNUC__
#define __USE_BSD 1
#endif
#include <sys/time.h>

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

