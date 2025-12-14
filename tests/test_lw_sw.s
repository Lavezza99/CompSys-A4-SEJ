# test_lw_sw.s - Test LW and SW
.globl _start
_start:
    la      x1, test_data
    li      x2, 0x12345678
    sw      x2, 0(x1)
    lw      x3, 0(x1)
    li      x4, 0xABCDEF00
    sw      x4, 4(x1)
    lw      x5, 4(x1)
    li      x6, 0x11111111
    li      x7, 0x22222222
    sw      x6, 8(x1)
    sw      x7, 12(x1)
    lw      x8, 8(x1)
    lw      x9, 12(x1)
    li      a7, 93
    ecall
.data
.align 4
test_data:
    .word   0, 0, 0, 0, 0, 0
