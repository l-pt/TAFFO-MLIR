program simple
implicit none

double precision, dimension(:,:), allocatable :: a
integer :: d = 500
integer :: i

allocate(a(d, d))
!call init_array(d, a)
deallocate(a)
!
!contains
!    subroutine init_array(m, mat)
!        implicit none
!        double precision, dimension(m, m) :: mat
!        integer :: m
!        integer :: i, j
!        do i = 1, m
!            do j = 1, m
!                mat(i, j) = i + j * m
!            end do
!        end do
!    end subroutine
end program
