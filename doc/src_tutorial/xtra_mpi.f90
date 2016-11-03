program main_mpi
	include 'mpif.h'
	call MPI_Init(ierr )
	call MPI_Comm_rank( MPI_COMM_WORLD, myid, ierr )
	call MPI_Comm_size( MPI_COMM_WORLD, ncpus, ierr )
	call MPI_Finalize( ierr )
	stop
end
