# test_sub.s - Test SUB instruction
.globl _start
_start:
    li      x1, 50
    li      x2, 20
    sub     x3, x1, x2
    li      x4, 10
    li      x5, 20
    sub     x6, x4, x5
    li      x7, 0
    li      x8, 5
    sub     x9, x7, x8
    li      a7, 93
    ecall
