#ifdef _PM_WITHOUT_MPI_
	program check
	write(6,'(a)') "This program is aimed for MPI Group only. Skip the test..."
	end
#else
	program check
	use omp_lib
	include 'mpif.h'
	parameter(msize=1000)
	!cx	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)

	double precision dptime_omp
	double precision dpt1,dpt0
	integer nWatch

	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
		write(*,*) "*** Allocate() failed."
		stop
	endif

	call mpi_init(ierr )
	call mpi_comm_rank( MPI_COMM_WORLD, myid, ierr )
	call mpi_comm_size( MPI_COMM_WORLD, ncpus, ierr )
	write(6,'(a,i3,a)') "fortran <main> started process:", myid

	n=msize
	nWatch=10
	call f_pm_initialize (nWatch)

	icalc=0         !cx 0:calc, 1:comm
	iexclusive=1    !cx 1:true, 2:false
	call f_pm_setproperties ("1-subinit", icalc, iexclusive)
	call f_pm_setproperties ("2-submtxm", icalc, iexclusive)

	call f_pm_start ("1-subinit")
	call subinit (msize,n,a,b,c)
	call f_pm_stop ("1-subinit", 0.0, 0)

	dpt0=dptime_omp()
	loops=3
	do i=1,loops
	call f_pm_start ("2-submtxm")
	call submtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("2-submtxm", 0.0, 0)
	end do

	dpt1=dptime_omp()
	s=dpt1-dpt0
	flops=real(loops)*dflop/s*1.e-9
	write(6,'(f10.5,a, f10.5,a)') s, " seconds", flops, " Gflops"
	write(11,*)  'output for unit 11', c(1,11), c(msize,msize)

	call f_pm_gather ()
	!cx call f_pm_print ("pmlib_report.txt")
	!cx call f_pm_printdetail ("pmlib_report.txt",1)
	call f_pm_print ("")
	call f_pm_printdetail ("",1)
	call MPI_Finalize( ierr )
	stop
	end

subroutine subinit (msize,n,a,b,c)
real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
!$omp parallel
!$omp do 
	do j=1, n
	do i=1, n
	a(i,j)=sin(real(i)/real(n))
	b(i,j)=cos(real(i)/real(n))
	end do
	end do
!$omp end do
!$omp end parallel
	return
end
	
subroutine submtxm (msize,n,dflop,a,b,c)
use omp_lib
real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
real(kind=8) :: x
	
	dflop=2.0*dble(n)**3
!$omp parallel
!$omp do private(x)
	do j=1, n
	do i=1, n
	x=0
	do k=1, n
	x=x+a(i,k)*b(k,j)
	end do
	c(i,j) = x
	end do
	end do
!$omp end do
!$omp end parallel
	return
end
	
	
double precision function dptime_omp()
!cx	use omp_lib
!cx	dptime_omp = omp_get_wtime()
	include 'mpif.h'
	dptime_omp = MPI_Wtime()
end
#endif
