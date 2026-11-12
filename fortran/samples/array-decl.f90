real function arraytest() result(rv)
        implicit none
        real :: myarray(32) = 1.1
        integer :: i
        rv = 0.0
        do i=0,31,1
                rv = rv + myarray(i)
        end do
        rv = rv + myarray(3)
end function arraytest
