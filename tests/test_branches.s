# test_branches.s - Test all branch instructions
.globl _start
_start:
    li      x1, 10
    li      x2, 10
    beq     x1, x2, beq_ok
    j       fail
beq_ok:
    li      x3, 5
    li      x4, 10
    bne     x3, x4, bne_ok
    j       fail
bne_ok:
    li      x5, 5
    li      x6, 10
    blt     x5, x6, blt_ok
    j       fail
blt_ok:
    li      x7, 10
    li      x8, 5
    bge     x7, x8, bge_ok
    j       fail
bge_ok:
    li      x9, 5
    li      x10, 10
    bltu    x9, x10, bltu_ok
    j       fail
bltu_ok:
    li      x11, 10
    li      x12, 5
    bgeu    x11, x12, bgeu_ok
    j       fail
bgeu_ok:
    li      x13, 5
    li      x14, 10
    beq     x13, x14, fail
    li      x15, 10
    li      x16, 10
    bne     x15, x16, fail
    j       success
fail:
    li      x31, 0xDEAD
    li      a7, 93
    ecall
success:
    li      x31, 0x600D
    li      a7, 93
    ecall
