module matrix_module
real(kind=8), allocatable :: a2(:,:)
real(kind=8), allocatable :: b2(:,:)
real(kind=8), allocatable :: c2(:,:)
integer matrix_size
end module


subroutine sub_initialize
	use matrix_module
	parameter (MAX_MATRIX=15000)
	integer(kind=8) i8n

	matrix_size=MAX_MATRIX
	n = matrix_size
	i8n = (n/1000)*(n/1000)*8*3
	allocate (a2(n,n), b2(n,n), c2(n,n), stat=ierr)
	write(*,*) "<sub_initialize> allocating", i8n, " MB of memory"
	if(ierr.eq.0) then
		write(*,*) "<allocate> was successful"
	else
		write(*,*) "*** Error. <allocate> failed: ierr=", ierr
	endif
!$omp parallel 
!$omp do 
	do j=1, n
	do i=1, n
	a2(i,j) = 1.0
	b2(i,j) = 2.0
	c2(i,j) = 3.0
	end do
	end do
!$omp end do
!$omp end parallel
	return
end
	
subroutine sub_dgemm
	use matrix_module
	real(kind=8) :: one, x

!cx	write(*,*) "<sub_dgemm> matrix size:", matrix_size

	if(.true.) then
	n = matrix_size
	one = 1.0
	call dgemm ("N", "N", n, n, n, one, a2, n, b2, n, one, c2, n)

	else
!$omp parallel 
!$omp do private(x)
	do j=1, n
	do i=1, n
	x = 0.0
	do k=1, n
	x = x + a2(i,k)*b2(k,j)
	end do
	c2(i,j) = x
	end do
	end do
!$omp end do
!$omp end parallel
	endif
	return
end

