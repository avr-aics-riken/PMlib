#if defined(__PGI) && (FORCE_CXX_MAIN)
subroutine fortmain
#else
program main
#endif

#if defined(DISABLE_MPI)
#else
	include 'mpif.h'
#endif
	parameter(msize=500)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)
	real(kind=8) :: dinit, dflop, dbyte
	integer nWatch
	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
		write(*,*) "*** Allocate() failed."
		stop
	endif
	myid=0
#if defined(DISABLE_MPI)
#else
	call MPI_Init(ierr )
	call MPI_Comm_rank( MPI_COMM_WORLD, myid, ierr )
	call MPI_Comm_size( MPI_COMM_WORLD, ncpus, ierr )
#endif
	write(6,'(a,i3,a)') "fortran <test4_main> started process:", myid
	n=msize
	nWatch=4
	call f_pm_initialize (nWatch)

	dinit=(n**2)*4.0
	dflop=(n**3)*4.0
	dbyte=(n**3)*4.0*3.0

	call f_pm_start ("Initial-section")
	call subinit (msize,n,a,b,c)
	call f_pm_stop_usermode ("Initial-section", dinit, 1)
	call spacer (msize,n,a,b,c)

	call f_pm_start ("Loop-section")

	do i=1,5
	if(myid.eq.0) write(1,'(a,i3,a)') "loop: i=",i

	call f_pm_start ("Kernel-Slow")
	call slowmtxm (msize,n,dflop,a,b,c)
	call f_pm_stop_usermode ("Kernel-Slow", dflop*4.0, 1)
	call spacer (msize,n,a,b,c)

	call f_pm_start ("Kernel-Fast")
	call submtxm (msize,n,dflop,a,b,c)
	call f_pm_stop_usermode ("Kernel-Fast", dflop, 1)
	call spacer (msize,n,a,b,c)

	!cx call f_pm_printprogress ("", "for checking", 0)
	!cx if(i.eq.4) then
	!cx call f_pm_resetall ()
	!cx endif

	end do

	call f_pm_stop_usermode ("Loop-section", dflop*6.0, 1)

!cx call f_pm_posttrace ()

	call f_pm_print ("", "", "", 0)
	call f_pm_printdetail ("", 0, 0)
    call f_pm_printthreads ("", 0, 0)
    call f_pm_printlegend ("")
#if defined(DISABLE_MPI)
#else
	call MPI_Finalize( ierr )
#endif
	write(6,'(a,i3,a)') "fortran <test4_main> finished process:", myid

	!cx	return
	!cx	end program
end

subroutine subinit (msize,n,a,b,c)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	!$omp parallel
	!$omp do
	do j=1, n
	do i=1, n
	a(i,j)=sin(real(i)/real(n))
	b(i,j)=cos(real(i)/real(n))
	c(i,j)=0.0
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
	x = x + a(k,i)*a(k,j) + b(k,i)*b(k,j)
	end do
	c(i,j) = c(i,j) + x/real(n)
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
!OCL NOSIMD
!OCL NOUNROLL
!OCL NOSWP
!DIR$ NOVECTOR
!DIR$ NOUNROLL
	do i=1, n
	x=0
!OCL NOSIMD
!OCL NOUNROLL
!OCL NOSWP
!DIR$ NOVECTOR
!DIR$ NOUNROLL
	do k=1, n
	x = x + a(k,i)*a(k,j) + b(k,i)*b(k,j)
	end do
	c(i,j) = c(i,j) + x/real(n)
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
