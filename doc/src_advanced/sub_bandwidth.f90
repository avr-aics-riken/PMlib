subroutine sub_copy (n,a,b,c)
	integer(kind=8) :: n
	real(kind=8) :: a(n),b(n),c(n)
!$omp parallel do
	do i=1,n
	c(i) = a(i)
	end do
end subroutine 

subroutine sub_triad (n,a,b,c,scale_factor)
	integer(kind=8) :: n
	real(kind=8) :: a(n),b(n),c(n)
!$omp parallel do
	do i=1,n
	c(i) = a(i) + b(i)*scale_factor
	end do
end subroutine 


!cx the following routines are not used for this test.
#ifdef USE_TIMER_API
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
