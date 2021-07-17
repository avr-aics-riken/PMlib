module pmlib_overload_api
contains
	subroutine f_pm_stop (fc, argX, argY)
	character*(*),intent(in)    ::  fc(*)
	real(8),intent(in),optional :: argX
	integer,intent(in),optional :: argY
	real(8) fpt
	integer itic
	if (present(argX) .and. present(argY)) then
		write(6,'(a,i3,a)') "check <ff_pm_stop> calling <f_pm_stop>"
		call f_pm_stop_usermode (fc, argX, argY)
	else
		write(6,'(a,i3,a)') "check <ff_pm_stop> arguments are omitted"
		call f_pm_stop_usermode (fc, 0.0, 1)
	endif
	return
	end
end module pmlib_overload_api

