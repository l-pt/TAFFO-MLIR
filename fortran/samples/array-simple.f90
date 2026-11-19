integer function arraytest() result(rv)
        implicit none
        integer :: myarray(-5:5)
        myarray(1) = 3
        rv = myarray(1)
end function arraytest
