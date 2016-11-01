program test4_main
#if defined(_PM_WITHOUT_MPI_)
#else
	include 'mpif.h'
#endif
	parameter(msize=1000)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)
	real(kind=8) :: dinit, dflop, dbyte
	integer nWatch
	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
		write(*,*) "*** Allocate() failed."
		stop
	endif
	myid=0
#if defined(_PM_WITHOUT_MPI_)
#else
	call MPI_Init(ierr )
	call MPI_Comm_rank( MPI_COMM_WORLD, myid, ierr )
	call MPI_Comm_size( MPI_COMM_WORLD, ncpus, ierr )
#endif
	write(6,'(a,i3,a)') "fortran <test4_main> started process:", myid
	n=msize
	nWatch=4
	call f_pm_initialize (nWatch)
	
	icalc=1
	icomm=0
	iexclusive=1
	iinclusive=0    !cx i.e. not exclusive
	call f_pm_setproperties ("First section", icalc, iexclusive)
	call f_pm_setproperties ("Second section", icalc, iinclusive)
	call f_pm_setproperties ("Subsection P", icomm, iexclusive)
	call f_pm_setproperties ("Subsection Q", icalc, iexclusive)
	
	dinit=(n**2)*4.0
	dflop=(n**3)*4.0
	dbyte=(n**3)*4.0*3.0
	
	call f_pm_start ("First section")
	call subinit (msize,n,a,b,c)
	call f_pm_stop ("First section", dinit, 1)
	call spacer (msize,n,a,b,c)
	
	call f_pm_start ("Second section")

	do i=1,3
	call f_pm_start ("Subsection P")
	call slowmtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("Subsection P", dflop*4.0, 1)
	call spacer (msize,n,a,b,c)
	
	call f_pm_start ("Subsection Q")
	call submtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("Subsection Q", dflop, 1)
	call spacer (msize,n,a,b,c)
	
!cx call f_pm_printprogress ("", "check point:", 0)
	end do
	
	call f_pm_stop ("Second section", dflop*6.0, 1)

!cx call f_pm_posttrace ()

	call f_pm_print ("", 0)
	call f_pm_printdetail ("", 0, 0)
#if defined(_PM_WITHOUT_MPI_)
#else
	call MPI_Finalize( ierr )
#endif
	write(6,'(a,i3,a)') "fortran <test4_main> finished process:", myid
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
	real(kind=8) :: dflop
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

subroutine slowmtxm (msize,n,dflop,a,b,c)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	real(kind=8) :: dflop
	real(kind=8) :: x
	dflop=2.0*dble(n)**3
	!$omp parallel
	!$omp do private(x)
	do j=1, n
	do i=1, n
	x=0
!OCL NOSIMD
!OCL NOUNROLL
!DIR$ NOVECTOR
!DIR$ NOUNROLL
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
	
subroutine spacer (msize,n,a,b,c)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	real(kind=8) :: dflop
	call slowmtxm (msize,n/2,dflop,a,b,c)
	return
end
	
#ifdef _PM_WITHOUT_MPI_
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
