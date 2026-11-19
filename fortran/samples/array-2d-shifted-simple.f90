integer function arraytest() result(rv)
        implicit none
        integer :: myarray(0:1, -1:1)
        myarray(0, -1) = 11
        myarray(0, 0) = 12
        myarray(1, -1) = 21
        myarray(1, 1) = 23
        rv = myarray(1, -1)
end function arraytest
