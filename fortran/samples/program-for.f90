program simple
implicit none
integer :: m = 10
integer :: i, j

j = 0
do i = 1, m
    j = j + i
end do
print *, j

do i = m, 1, -1
    j = j - i
end do
print *, j

do i = 5, 5
    j = j + i
end do
print *, j

do i = 1, m, -1
    j = j + i
end do
print *, j
end program
