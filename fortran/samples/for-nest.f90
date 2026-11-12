integer function testfor() result(rv)
        implicit none
        integer :: i, j
        rv = 0
        do i=1,10,1
                do j=3,30,3
                        rv = rv + i + j;
                end do
        end do
end function testfor
