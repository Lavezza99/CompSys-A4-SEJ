# test_loads.s - Test all load instructions
.globl _start
_start:
    la      x1, test_data
    lw      x2, 0(x1)
    lh      x3, 0(x1)
    lh      x4, 2(x1)
    lb      x5, 0(x1)
    lb      x6, 1(x1)
    lb      x7, 2(x1)
    lb      x8, 3(x1)
    lhu     x9, 0(x1)
    lhu     x10, 2(x1)
    lbu     x11, 0(x1)
    lbu     x12, 1(x1)
    lw      x13, 4(x1)
    lb      x14, 4(x1)
    lbu     x15, 4(x1)
    li      a7, 93
    ecall
.data
.align 4
test_data:
    .word   0x12345678
    .word   0xFFFFFFFF
    .word   0xABCDEF00
