integer function testfor() result(rv)
        implicit none
        integer :: i
        rv = 0
        do i=1,10,1
                rv = rv + i;  
        end do
end function testfor
