#if defined(__PGI) && (FORCE_CXX_MAIN)
subroutine fortmain
#else
program main
#endif

use omp_lib
#ifdef DISABLE_MPI
my_id=777
#else
include 'mpif.h'
call mpi_init(ierr )
!cx irequired=MPI_THREAD_MULTIPLE
!cx call mpi_init_thread(irequired, isupport, ierr )
call mpi_comm_rank( MPI_COMM_WORLD, my_id, ierr )
call mpi_comm_size( MPI_COMM_WORLD, npes, ierr )
#endif

!$omp parallel private(ip,x,i)

ip=omp_get_thread_num()
call f_pm_initialize (10)
icalc=1
icomm=0
iexclusive=1
iinclusive=0
call f_pm_setproperties ("Section-A", icalc, iexclusive)
call f_pm_setproperties ("Section-B", icalc, iexclusive)

do i=1,2
call f_pm_start ("Section-A")
call check_thread(x)
call f_pm_stop  ("Section-A", 111.0d0, 1)
write(0,'(a,2i3,e12.2)') "Section-A rank, thread, x=", my_id, ip, x
if(mod(my_id+ip,6).eq.1) then
    call f_pm_start ("Section-B")
    call check_thread(x)
    call f_pm_stop  ("Section-B", 7.0d0, 1)
    write(0,'(a,2i3,e12.2)') "Section-B rank, thread, x=", my_id, ip, x
endif
end do
call f_pm_mergethreads ()

!$omp end parallel

call f_pm_setproperties ("Section-C", icalc, iexclusive)
call f_pm_start ("Section-C")
call check_thread(x)
call f_pm_stop  ("Section-C", 111.0d0, 1)

call f_pm_print ("", 1)
call f_pm_printdetail ("", 0, 1)
id=0
call f_pm_printthreads ("", id, 1)

#if !defined(DISABLE_MPI)
call MPI_Finalize( ierr )
#endif
#if defined(__PGI) && (FORCE_CXX_MAIN)
return
#endif
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

