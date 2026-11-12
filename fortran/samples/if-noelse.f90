integer function iftest() result(rv)
        implicit none
        integer :: xyzzy
        xyzzy = 77
        rv = 44
        if (xyzzy > 12) then
                rv = 10
        end if
end function iftest
