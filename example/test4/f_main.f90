	program check
	use omp_lib
	parameter(msize=2000)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)

	include 'mpif.h'
	double precision dptime_omp
	double precision dpt1,dpt0
	integer nWatch

	call mpi_init(ierr )
	call mpi_comm_rank( MPI_COMM_WORLD, MYID, ierr )
	call mpi_comm_size( MPI_COMM_WORLD, ncpus, ierr )

	n=msize
    nWatch=10
	call f_pm_initialize (nWatch)

	icalc=0         !cx 0:calc, 1:comm
	iexclusive=1    !cx 1:true, 2:false
	call f_pm_setproperties ("1-subinit", icalc, iexclusive)
	call f_pm_setproperties ("2-submtxm", icalc, iexclusive)

	call f_pm_start ("1-subinit")
	call subinit (msize,n,a,b,c)
	call f_pm_stop ("1-subinit")

	dpt0=dptime_omp()
	loops=3
	do i=1,loops
	call f_pm_start ("2-submtxm")
	call submtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("2-submtxm")
	end do

	dpt1=dptime_omp()
	s=dpt1-dpt0
	flops=real(loops)*dflop/s*1.e-6
	write(6,'(f10.5,a, f10.2,a)') s, " seconds", flops, " MFLOPS"
	write(11,*)  'output for unit 11', c(1,11), c(msize,msize)

	call f_pm_gather ()
	call f_pm_print ()
	call f_pm_printdetail ()
	call MPI_Finalize( ierr )
	stop
	end

subroutine subinit (msize,n,a,b,c)
real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	do i=1, n
	do j=1, n
	a(j,i)=sin(real(j)/n)
	b(j,i)=cos(real(j)/n)
	end do
	end do
	return
end
	
subroutine submtxm (msize,n,dflop,a,b,c)
use omp_lib
real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
real(kind=8) :: x
	
	dflop=2.0*dble(n)**3
!$omp parallel
!$omp do private(x)
	do i=1, n
	do j=1, n
	x=0
	do k=1, n
	x=x+a(k,i)*b(k,j)
	end do
	c(i,j) = x
	end do
	end do
!$omp end do
!$omp end parallel
	return
end
	
	
double precision function dptime_omp()
	use omp_lib
	dptime_omp = omp_get_wtime()
!cx include 'mpif.h'
!cx	dptime_omp = MPI_Wtime()
end
