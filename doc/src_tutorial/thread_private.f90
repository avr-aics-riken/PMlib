program fort_thread
include 'mpif.h'
call mpi_init(ierr )
call mpi_comm_rank( MPI_COMM_WORLD, my_id, ierr )
call mpi_comm_size( MPI_COMM_WORLD, npes, ierr )

ixp=omp_get_thread_num()
nxp=omp_get_num_threads()
num_threads=omp_get_max_threads()

nWatch=num_threads

!$omp parallel
call f_pm_initialize (1)
icalc=1
icomm=0
iexclusive=1
iinclusive=0    !cx i.e. not exclusive
call f_pm_setproperties ("Section-A", icalc, iexclusive)
call f_pm_setproperties ("Section-B", icalc, iexclusive)

do i=1,1
call f_pm_start ("Section-A")
call check_thread(i,n,x)
call f_pm_stop  ("Section-A", 0.0, 1)
write(0,'(a,2i3,e12.2)') "somekernel rank, thread, x=", my_id, ixp, x
call f_pm_start ("Section-B")
call f_pm_stop  ("Section-B", 0.0, 1)
end do
!$omp barrier
call f_pm_mergethreads()
!$omp end parallel

call f_pm_setproperties ("Section-C", icalc, iexclusive)
call f_pm_start ("Section-C")
call check_thread(i,n,x)
call f_pm_stop  ("Section-C", 0.0, 1)

call f_pm_print ("", 1)
call f_pm_printdetail ("", 0, 1)
call f_pm_printthreads ("", 0, 1)

call MPI_Finalize( ierr )

stop
end

subroutine check_thread(iarg,n,x)
    !cx use omp_lib

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

write(6,'(a,2e15.3)') "<check_thread> finish. x=", x
return
end subroutine

