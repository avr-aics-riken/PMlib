program fort_thread
use omp_lib

call mpi_init(ierr )
!cx     irequired=MPI_THREAD_MULTIPLE
!cx     call mpi_init_thread(irequired, isupport, ierr )
call mpi_comm_rank( MPI_COMM_WORLD, my_id, ierr )
call mpi_comm_size( MPI_COMM_WORLD, npes, ierr )

ixp=omp_get_thread_num()
nxp=omp_get_num_threads()
num_threads=omp_get_max_threads()

write(0,'(a,3i5)') "<main> starting ixp, nxp, num_threads=", ixp, nxp, num_threads

call f_pm_initialize (10)
call f_pm_start ("WorkSharing-A")
call check_thread(x)
call f_pm_stop  ("WorkSharing-A")
call f_pm_report ("")   !cx Most users should be OK with this
!cx call f_pm_report ("PMlib-report")   !cx If needed, the report can be written to a separate file.

call MPI_Finalize( ierr )

stop
end

subroutine check_thread(x)

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

write(6,'(a,2e15.3)') "<check_thread> finish. x=", x
return
end subroutine

