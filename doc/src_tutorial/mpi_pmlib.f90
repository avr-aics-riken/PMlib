program check
#ifndef DISABLE_MPI
	include 'mpif.h'
#endif
	parameter(msize=1000)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)
	double precision dptime_omp
	double precision dpt1,dpt0
	integer nWatch

	myid=0
	ncpus=1
#ifndef DISABLE_MPI
	call mpi_init(ierr )
	call mpi_comm_rank( MPI_COMM_WORLD, myid, ierr )
	call mpi_comm_size( MPI_COMM_WORLD, ncpus, ierr )
	write(6,'(a,i3,a)') "MPI <main> started process:", myid
#else
	write(6,'(a,i3,a)') "Serial <main> started"
#endif
	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
		write(*,*) "*** Allocate() failed."
		stop
	endif

	n=msize
	nWatch=10
	call f_pm_initialize (nWatch)

	icalc=1         !cx 1:calc, 0:comm
	iexclusive=1    !cx 1:true, 2:false
	call f_pm_setproperties ("1st section", icalc, iexclusive)
	call f_pm_setproperties ("2nd section", icalc, iexclusive)

	call f_pm_start ("1st section")
	call subinit (msize,n,a,b,c)
	call f_pm_stop ("1st section", 0.0, 0)

	dpt0=dptime_omp()
	loops=3
	do i=1,loops
	call f_pm_start ("2nd section")
	call submtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("2nd section", dflop, 0)
	end do
	dpt1=dptime_omp()
	flops=real(loops)*dflop/(dpt1-dpt0)*1.e-9
	write(6,'(f10.5,a, f10.5,a)') dpt1-dpt0, " seconds", flops, " Gflops"

	call f_pm_gather ()
	call f_pm_print ("", 0)
	call f_pm_printdetail ("", 0, 0)
	do i=1,ncpus
	call f_pm_printthreads ("", i-1, 0)
	end do
	call f_pm_printlegend ("")
#ifndef DISABLE_MPI
	call MPI_Finalize( ierr )
	write(*,*)  'finishing <main> rank', myid, ' computed', c(myid+1,myid+1)
#else
	write(*,*)  'finishing <main> computed', c(myid+1,myid+1)
#endif
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
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	real(kind=8) :: x
	dflop=2.0*dble(n)**3
	!$omp parallel private(i,j,k, x, c1,c2,c3)
	!$omp do 
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


#ifdef DISABLE_MPI
	#ifdef _OPENMP
double precision function dptime_omp()
	use omp_lib
	dptime_omp = omp_get_wtime()
	end
	#else
double precision function dptime_omp()
	integer(8) :: cv,cr
	call system_clock(count=cv,count_rate=cr)
	dptime_omp = real(cv,8)/real(cr,8)
	end
	#endif
#else
double precision function dptime_omp()
	include 'mpif.h'
	dptime_omp = MPI_Wtime()
	end
#endif
