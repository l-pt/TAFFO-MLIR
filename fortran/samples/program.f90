program simple
implicit none
integer :: xyzzy = 44;
integer :: rv
rv = frob(xyzzy)
if (rv /= 47) stop 1
contains
        integer function frob(i) result(rv)
                implicit none
                integer :: i
                rv = i + 3
        end function
end program
