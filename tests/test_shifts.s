# test_shifts.s - Test shift operations
.globl _start
_start:
    li      x1, 1
    slli    x2, x1, 4
    li      x3, 128
    srli    x4, x3, 3
    li      x5, 64
    srai    x6, x5, 2
    li      x7, -16
    srai    x8, x7, 1
    li      x9, 1
    li      x10, 5
    sll     x11, x9, x10
    li      x12, 64
    li      x13, 2
    srl     x14, x12, x13
    li      x15, -32
    li      x16, 2
    sra     x17, x15, x16
    li      a7, 93
    ecall
