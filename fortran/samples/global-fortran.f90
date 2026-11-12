function test() result(x)
  integer :: x
  integer, save :: global = 42

  x = global
end function test
