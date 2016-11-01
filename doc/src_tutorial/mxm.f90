program check
	parameter(msize=1000)
	real(kind=8), allocatable :: a(:,:),b(:,:),c(:,:)
	
	allocate (a(msize,msize), b(msize,msize), c(msize,msize), stat=istat)
	if (istat.ne.0) then
	    stop "*** Allocate() failed."
	endif
	
    !uncomment	nWatch=10
    !uncomment	call f_pm_initialize (nWatch)
	
	n=msize
	call subinit2d (msize,n,a,b,c)
	
    !uncomment	icalc=0         !cx 0:calc, 1:comm
    !uncomment	iexclusive=1    !cx 1:true, 2:false
    !uncomment	call f_pm_setproperties ("Label-1", icalc, iexclusive)
	
    !uncomment	call f_pm_start ("Label-1")
	call submxm2d (msize,n,dflop,a,b,c)
    !uncomment	call f_pm_stop ("Label-1", 0.0, 0)
	
    !uncomment	call f_pm_print ("")
    !uncomment	call f_pm_printdetail ("",0)
	write(*,*)  'something was computed', c(msize,msize)
	
	stop
end
	
subroutine subinit2d (msize,n,a,b,c)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	!uncomment  !$omp parallel do 
	do j=1, n
	do i=1, n
	a(i,j)=sin(real(i)/real(n))
	b(i,j)=cos(real(i)/real(n))
	end do
	end do
	return
end
	
subroutine submxm2d (msize,n,dflop,a,b,c)
	real(kind=8) :: a(msize,msize), b(msize,msize), c(msize,msize)
	real(kind=8) :: x
	
	dflop=2.0*dble(n)**3
	!uncomment  !$omp parallel do private(x)
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
end
