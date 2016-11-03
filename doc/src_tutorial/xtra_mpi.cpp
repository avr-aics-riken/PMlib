#include <mpi.h>
int main (int argc, char *argv[])
{
	int my_id, npes;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &npes);
	MPI_Finalize();
	return 0;
}

