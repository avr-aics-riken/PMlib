program bandwidth

	include 'mpif.h'
	integer(kind=8) :: maxsize, n
	real(kind=8), allocatable :: a(:),b(:),c(:)
	real(kind=8) :: scale_factor

	maxsize=1000000000/8
	n=maxsize
	my_id=0
	call MPI_Init(ierr )
	call MPI_Comm_rank( MPI_COMM_WORLD, my_id, ierr )
	call MPI_Comm_size( MPI_COMM_WORLD, n_procs, ierr )
	if (my_id.eq.0) then
	write(6,'(a,f5.2,a)') "<program bandwidth>"
	write(6,'(a,f5.2,a)') "process bandwidth for", real(n*8)*1.0e-9, " GB arrays"
	write(6,'(a,2es10.2)') "COPY  Bytes RD & WR:", real(8*n*1), real(8*n*1)
	write(6,'(a,2es10.2)') "TRIAD Bytes RD & WR:", real(8*n*2), real(8*n*1)
	endif

	allocate (a(n),stat=istat1)
	allocate (b(n),stat=istat2)
	allocate (c(n),stat=istat3)
	if ( (istat1+istat2+istat3).ne.0) then
		write(*,*) "*** Allocate() failed for process", my_id
		call MPI_Finalize( ierr )
		stop
	endif

	call f_pm_initialize (10)
	
	call f_pm_start ("Preset")
	scale_factor=1.0
	!$omp parallel do
	do i=1, n
	a(i)=real(i)
	b(i)=real(n-i)
	c(i)=0.0
	end do
	call f_pm_stop ("Preset", 8*n*3, 1)

	call f_pm_start ("COPY-kernel")
	call sub_copy (n,a,b,c)
	call f_pm_stop  ("COPY-kernel")

	call f_pm_start ("TRIAD-kernel")
	call sub_triad (n,a,b,c,scale_factor)
	call f_pm_stop  ("TRIAD-kernel")

	call f_pm_report ("")

	call MPI_Finalize( ierr )
end

