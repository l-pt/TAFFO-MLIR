integer function test() result(y)
  implicit none
  integer :: x
  integer, save :: global = 42

  x = global
  y = x + 10
end function test
