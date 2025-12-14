# test_add.s - Test ADD instruction
.globl _start
_start:
    li      x1, 10
    li      x2, 20
    add     x3, x1, x2
    li      x4, 0
    add     x5, x3, x4
    li      x6, -1
    li      x7, 1
    add     x8, x6, x7
    li      a7, 93
    ecall
