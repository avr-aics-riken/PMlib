#ifdef _PM_WITHOUT_MPI_
program test4_main
	parameter(msize=1000)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)
	real(kind=8) :: dflop

	write(6,'(a)') "<test4_main> starting."
	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
		write(*,*) "*** Allocate() failed."
		stop
	endif

	n=msize
	nWatch=10
	call f_pm_initialize (nWatch)
	
	icalc=1
	icomm=0
	iexclusive=1
	iinclusive=0    !cx i.e. not exclusive
	call f_pm_setproperties ("First section", icalc, iexclusive)
	call f_pm_setproperties ("Second section", icalc, iinclusive)
	call f_pm_setproperties ("Subsection X", icomm, iexclusive)
	call f_pm_setproperties ("Subsection Y", icalc, iexclusive)
	
	call f_pm_start ("First section")
	call subinit (msize,n,a,b,c)
	dflop=(n**2)*4.0
	call f_pm_stop ("First section", dflop, 1)
	
	call spacer (msize,n,a,b,c)
	
	call f_pm_start ("Second section")
	do i=1,3
	call f_pm_start ("Subsection X")
	call slowmtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("Subsection X", dflop*4.0, 1)
	call spacer (msize,n,a,b,c)
	
	call f_pm_start ("Subsection Y")
	call submtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("Subsection Y", dflop, 1)
	call spacer (msize,n,a,b,c)
	end do
	call f_pm_stop ("Second section", dflop, 1)
	
	write(6,'(a)') "<test4_main> finished computing submxm."
	
	call f_pm_gather ()
	call f_pm_print ("", 0)
	call f_pm_printdetail ("", 0, 0)
	stop
end

#else
program test4_main
	include 'mpif.h'
	parameter(msize=1000)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)
	real(kind=8) :: dflop

	integer nWatch

	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
		write(*,*) "*** Allocate() failed."
		stop
	endif

	call mpi_init(ierr )
	call mpi_comm_rank( MPI_COMM_WORLD, myid, ierr )
	call mpi_comm_size( MPI_COMM_WORLD, ncpus, ierr )
	write(6,'(a,i3,a)') "fortran <test4_main> started process:", myid

	n=msize
	nWatch=10
	call f_pm_initialize (nWatch)
	
	icalc=1
	icomm=0
	iexclusive=1
	iinclusive=0    !cx i.e. not exclusive
	call f_pm_setproperties ("First section", icalc, iexclusive)
	call f_pm_setproperties ("Second section", icalc, iinclusive)
	call f_pm_setproperties ("Subsection X", icomm, iexclusive)
	call f_pm_setproperties ("Subsection Y", icalc, iexclusive)
	
	call f_pm_start ("First section")
	call subinit (msize,n,a,b,c)
	dflop=(n**2)*4.0
	call f_pm_stop ("First section", dflop, 1)
	write(6,'(a,i3,a)') "<test4_main> finished process:", myid
	
	call spacer (msize,n,a,b,c)
	
	call f_pm_start ("Second section")
	do i=1,3
	call f_pm_start ("Subsection X")
	call slowmtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("Subsection X", dflop*4.0, 1)
	call spacer (msize,n,a,b,c)
	
	call f_pm_start ("Subsection Y")
	call submtxm (msize,n,dflop,a,b,c)
	call f_pm_stop ("Subsection Y", dflop, 1)
	call spacer (msize,n,a,b,c)
	end do
	call f_pm_stop ("Second section", dflop*6.0, 1)
	
	write(6,'(a,i3,a)') "fortran <test4_main> finished process:", myid
	
	call f_pm_gather ()
	call f_pm_print ("", 0)
	call f_pm_printdetail ("", 0, 0)
	call MPI_Finalize( ierr )
	stop
end
#endif

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
