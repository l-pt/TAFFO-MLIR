program simple
implicit none
integer :: m = 10
double precision :: matrix(m, m)

call init_array(m, matrix)
call print_array(m, matrix)

print *, matrix(1, 3)
print *, "Test"

contains
    subroutine print_array(m, mat)
        implicit none
        double precision :: mat(m, m)
        integer :: m
        integer :: i, j
        do i = 1, m
        do j = 1, m
        write(0, "(f0.2,1x)", advance='no') mat(j, i)
        if (mod(((i - 1) * m) + j - 1, m) == 0) then
            write(0, *)
        end if
        end do
        end do
        write(0, *)
    end subroutine
    subroutine init_array(m, mat)
        implicit none
        double precision :: mat(m, m)
        integer :: m
        integer :: i, j
        do i = 1, m
        do j = 1, m
            mat(i, j) = i + j * m
        end do
        end do
        write(0, *)
    end subroutine
end program
