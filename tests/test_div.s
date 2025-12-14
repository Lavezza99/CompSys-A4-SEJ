# test_div.s - Test DIV and REM
.globl _start
_start:
    li      x1, 100
    li      x2, 10
    div     x3, x1, x2
    li      x4, 17
    li      x5, 5
    div     x6, x4, x5
    rem     x7, x4, x5
    li      x8, 5
    li      x9, 10
    div     x10, x8, x9
    li      x11, -20
    li      x12, 4
    div     x13, x11, x12
    li      x14, 100
    li      x15, 10
    divu    x16, x14, x15
    li      x17, 17
    li      x18, 5
    remu    x19, x17, x18
    li      a7, 93
    ecall
