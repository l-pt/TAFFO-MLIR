integer function iftest() result(rv)
        implicit none
        integer :: xyzzy
        xyzzy = 77
        if (xyzzy > 12) then
                if (xyzzy > 32) then
                        rv = 44
                else
                        rv = 10
                end if
        else
                rv = 20
        end if
end function iftest
