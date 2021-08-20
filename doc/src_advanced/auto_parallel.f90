program auto_parallel
real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)

n=1000

write(6,'(a,i5)') "<main> allocating n=", n
allocate (a(n,n), b(n,n), c(n,n), stat=istat)
if (istat.ne.0) then
	write(*,*) "*** Allocate() failed."
	stop
endif

!cx call mpi_init(ierr )
!cx call mpi_comm_rank( MPI_COMM_WORLD, my_id, ierr )
!cx call mpi_comm_size( MPI_COMM_WORLD, npes, ierr )

ixp=omp_get_thread_num()
nxp=omp_get_num_threads()
num_threads=omp_get_max_threads()

call f_pm_initialize (10)
call f_pm_start ("WorkSharing-A")
call sub_init(a,b,c,n)
call f_pm_stop  ("WorkSharing-A")
call f_pm_start ("WorkSharing-B")
call submtxm(a,b,c,n)
call f_pm_stop  ("WorkSharing-B")
call f_pm_report ("")

!cx call MPI_Finalize( ierr )

stop
end

subroutine sub_init(a,b,c,n)
	real(kind=8) :: a(n,n), b(n,n), c(n,n)
	write(6,'(a,i5)') "<sub_init> initializing"
	do j=1, n
	do i=1, n
	a(i,j)=sin(real(i)/real(n))
	b(i,j)=cos(real(i)/real(n))
	end do
	end do

return
end subroutine

subroutine submtxm (a,b,c,n)
	real(kind=8) :: a(n,n), b(n,n), c(n,n)
	real(kind=8) :: x
	write(6,'(a,i3,a)') "<submtxm> starting"
	!cx !$omp parallel
	!cx !$omp do private(x)
	do j=1, n
	do i=1, n
	x=0
	do k=1, n
	x=x+a(i,k)*b(k,j)
	end do
	c(i,j) = x
	end do
	end do
	!cx !$omp end do
	!cx !$omp end parallel
	write(6,'(a,i3,a)') "<submtxm> returns"
	return
end


subroutine check_thread(x)
write(6,'(a,2e15.3)') "<check_thread> finish. x=", x
n=10000
x=0.0
!$omp parallel do private(j,k,y) shared(n) reduction(+:x)
do j=1, n
y=0.0
do k=1, n
y=y+ real(k)**1.5
end do
x=x+y
end do
!$omp end parallel do
return
end subroutine

