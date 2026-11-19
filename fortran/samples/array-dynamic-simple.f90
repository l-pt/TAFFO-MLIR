integer function arraytest(n, m) result(rv)
        implicit none
        integer, intent(in) :: n, m
        integer :: myarray(n, m)
        myarray(1, 2) = 12
        rv = myarray(1, 2)
end function arraytest
