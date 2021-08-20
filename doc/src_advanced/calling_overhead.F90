program calling_overhead
	write(6,'(a,i3,a)') "measuring the overhead of PMlib calls"
	nWatch=3
	call f_pm_initialize (nWatch)
	
	icalc=1
	iexclusive=1
	call f_pm_setproperties ("section", icalc, iexclusive)
	
	do i=1,100000
	call f_pm_start ("section")
	call f_pm_stop ("section")
	end do
	
	call f_pm_report ("")
	stop
end

