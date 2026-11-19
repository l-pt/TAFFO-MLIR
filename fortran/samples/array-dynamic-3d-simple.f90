integer function arraytest(x, y, z) result(rv)
        implicit none
        integer, intent(in) :: x, y, z
        integer :: myarray(x, y, z)
        myarray(1, 2, 3) = 123
        rv = myarray(1, 2, 3)
end function arraytest
