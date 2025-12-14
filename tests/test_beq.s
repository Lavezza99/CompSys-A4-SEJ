# test_beq.s - Test BEQ instruction
.globl _start
_start:
    li      x1, 10
    li      x2, 10
    beq     x1, x2, taken1
    li      x3, 0
    j       fail
taken1:
    li      x3, 1
    li      x4, 5
    li      x5, 10
    beq     x4, x5, fail
    li      x6, 2
    li      x7, 0
    li      x8, 0
    beq     x7, x8, taken2
    j       fail
taken2:
    li      x9, 3
    j       success
fail:
    li      x31, 0xDEAD
    li      a7, 93
    ecall
success:
    li      x31, 0x600D
    li      a7, 93
    ecall
