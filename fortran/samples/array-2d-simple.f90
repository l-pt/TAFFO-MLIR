integer function arraytest() result(rv)
        implicit none
        integer :: myarray(2, 3)
        myarray(1, 1) = 11
        myarray(1, 2) = 12
        myarray(2, 1) = 21
        myarray(2, 3) = 23
        rv = myarray(2, 1)
end function arraytest
