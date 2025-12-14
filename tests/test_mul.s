# test_mul.s - Test MUL instruction
.globl _start
_start:
    li      x1, 10
    li      x2, 20
    mul     x3, x1, x2
    li      x4, 100
    li      x5, 0
    mul     x6, x4, x5
    li      x7, 42
    li      x8, 1
    mul     x9, x7, x8
    li      x10, -5
    li      x11, 3
    mul     x12, x10, x11
    li      a7, 93
    ecall
