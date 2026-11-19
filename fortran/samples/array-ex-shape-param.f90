subroutine foo(n, m, a)
    implicit none
    integer, intent(in) :: n, m
    integer, intent(inout) :: a(n, m)

    a(1, 1) = 42
end subroutine foo

integer function test()
    integer :: a(3, 4)

    call foo(3, 4, a)
    test = a(1, 1)
end function test
