program main
use omp_lib
include 'mpif.h'

call mpi_init(ierr )
!cx irequired=MPI_THREAD_MULTIPLE
!cx call mpi_init_thread(irequired, isupport, ierr )
call mpi_comm_rank( MPI_COMM_WORLD, my_id, ierr )
call mpi_comm_size( MPI_COMM_WORLD, npes, ierr )

ip=omp_get_thread_num()
np=omp_get_num_threads()
num_threads=omp_get_max_threads()
write(0,'(a,3i3)') "<main> outside parallel region np, num=", np, num

!$omp parallel private(ip,x,i)
ip=omp_get_thread_num()
np=omp_get_num_threads()
num_threads=omp_get_max_threads()
write(0,'(a,3i3)') "<main> inside of parallel region ip, np, num=", ip, np, num

call f_pm_initialize (10)
do i=1,3
call f_pm_start ("Section-A")
call check_thread(x)
call f_pm_stop  ("Section-A")
end do
!$omp end parallel

call f_pm_report ("")
call MPI_Finalize( ierr )
stop
end

subroutine check_thread(x)
use omp_lib
n=10000
x=0.0

!$omp parallel do private(j,k,y) shared(n) reduction(+:x)
do j=1, n
y=0.0
do k=1, n
    !cx y=y+ real(k)**1.5
y=y+ real(k)*real(j)
end do
x=x+y
end do

return
end subroutine

