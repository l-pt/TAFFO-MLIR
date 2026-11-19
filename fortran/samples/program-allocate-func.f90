program simple
implicit none

call init_array()

contains
    subroutine init_array()
        double precision, dimension(:,:), allocatable :: a
        integer :: d = 500
        integer :: i
        allocate(a(d, d))
        deallocate(a)
    end subroutine
end program
