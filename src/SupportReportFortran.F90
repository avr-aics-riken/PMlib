!> PMlib report controll routine
!! - [1] stop the Root section
!! - [2] merge thread serial/parallel sections
!! - [3] select the type of the report and start producing the report
!!
subroutine f_pm_report (filename)
character(*) filename
integer id, mid, inside, nSections

!cx stop the Root section before report

id=0
call f_pm_serial_parallel (id, mid, inside)
if (inside.eq.0) then
    call f_pm_stop_Root ()
else if(inside.eq.1) then
    !$omp parallel
    call f_pm_stop_Root ()
    !$omp end parallel
else
    continue
endif

!cx count the number of SHARED sections

call f_pm_sections (nSections)

!cx merge thread data into the master thread 

do id=0,nSections-1

call f_pm_serial_parallel (id, mid, inside)
if (inside.eq.0) then
    !cx The section is defined outside of parallel context
    call f_pm_mergethreads (id)
else if(inside.eq.1) then
    !cx The section is defined inside parallel context
    !cx If an OpenMP parallel region is started by a Fortran routine,
    !cx the merge operation must be triggered by a Fortran routine,
    !cx which is outside of PMlib C++ class parallel context
    !cx The followng OpenMP parallel block profives such merging support.
    !$omp parallel shared(id)
    call f_pm_mergethreads (id)
    !$omp end parallel
else
    continue
endif
end do

!cx now start reporting the PMlib stats

call f_pm_select_report (filename)

return
end

