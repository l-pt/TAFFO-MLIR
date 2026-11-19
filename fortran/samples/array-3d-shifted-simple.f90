integer function arraytest() result(rv)
        implicit none
        integer :: myarray(0:1, -1:1, 5:9) ! 2 x 3 x 5
        myarray(0, -1, 5) = 111
        myarray(0, 0, 6) = 122
        myarray(1, -1, 7) = 213
        myarray(1, 1, 9) = 235
        myarray(1, -1, 6) = 212
        rv = myarray(1, -1, 6)
end function arraytest
