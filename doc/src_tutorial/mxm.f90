program main
	parameter(msize=1000)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)
	
	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
	    stop "*** Allocate() failed."
	endif
	n=msize
	
    !!	ninit=10
    !!	call f_pm_initialize (ninit)
	
    !!	call f_pm_start ("A:subinit2d")
	call subinit2d (msize,n,a,b,c)
    !!	call f_pm_stop ("A:subinit2d")
	
    !!	call f_pm_start ("B:submxm2d")
	call submxm2d (msize,n,dflop,a,b,c)
    !!	call f_pm_stop ("B:submxm2d")
	
    !!	call f_pm_report ("")
	write(*,*)  'something was computed', c(msize,msize)
	
	stop
end program
	
subroutine subinit2d (msize,n,a,b,c)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	!!  !$omp parallel do 
	do j=1, n
	do i=1, n
	a(i,j)=sin(real(i)/real(n))
	b(i,j)=cos(real(i)/real(n))
	end do
	end do
	return
end subroutine
	
subroutine submxm2d (msize,n,dflop,a,b,c)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	real(kind=8) :: x
	
	dflop=2.0*dble(n)**3
	!!  !$omp parallel do private(i,j,k,x)
	do j=1, n
	do i=1, n
	x=0
	do k=1, n
	x=x+a(i,k)*b(k,j)
	end do
	c(i,j) = x
	end do
	end do
	return
end subroutine
