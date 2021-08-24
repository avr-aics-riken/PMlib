program fort_thread
include 'mpif.h'
call mpi_init(ierr )
call mpi_comm_rank( MPI_COMM_WORLD, my_id, ierr )
call mpi_comm_size( MPI_COMM_WORLD, npes, ierr )

!cx if the program calls PMlib inside of OpenMP parallel context, call initialize() in parallel
!cx otherwise, just call initialize() in a serial location

ninit=5
!$omp parallel
call f_pm_initialize (ninit)
!$omp end parallel

call f_pm_start ("Serial")
call single_thread(i,n,x)
call f_pm_stop  ("Serial")

!$omp parallel private (ith, nth, max_threads, i, n, x) 
ith=omp_get_thread_num()
nth=omp_get_num_threads()
max_threads=omp_get_max_threads()
!$omp do schedule(dynamic)
!cx !$omp do schedule(static,3)
do i=1,50
call f_pm_start ("ParaThreads")
call single_thread(i,n,x)
call f_pm_stop  ("ParaThreads")
end do
!$omp end do
!$omp end parallel
write(6,'(a,i3,3x,a)') "<<main>> rank", my_id, "arrived POINT 1"

call f_pm_start ("WorkSharing")
call work_sharing(i,n,x)
call f_pm_stop  ("WorkSharing")

write(6,'(a,i3,3x,a)') "<<main>> rank", my_id, "arrived POINT 2"
call MPI_Barrier( MPI_COMM_WORLD, ierr )
call f_pm_report ("")

call MPI_Barrier( MPI_COMM_WORLD, ierr )
write(6,'(a,i3,3x,a)') "<<main>> rank", my_id, "calling MPI_Finalize"
call MPI_Finalize( ierr )

write(6,'(a,i3,3x,a)') "<<main>> rank", my_id, "now ends"
stop
end


subroutine single_thread(ith,n,x)

n=100
x=0.0

do j=1, n
y=0.0
do k=1, n
y=y+ real(k)**1.5
end do
x=x+y
end do

write(6,'(a,i3,e15.3)') "<<single_thread>> thread finish. ith and x=", ith, x
return
end subroutine


subroutine work_sharing(n,x)
n=1000
x=0.0

!$omp parallel do private(j,k,y) shared(n) reduction(+:x)
do j=1, n
y=0.0
do k=1, n
y=y+ real(k)**1.5
end do
x=x+y
end do

write(6,'(a,e15.3)') "<<work_sharing>> finish. x=", x
return
end subroutine

