integer function iftest() result(rv)
        implicit none
        integer :: xyzzy
        xyzzy = 77
        if (xyzzy > 12) then
                rv = 10
        else
                rv = 20
        end if
end function iftest
