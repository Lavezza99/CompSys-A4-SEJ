# test_stores.s - Test all store instructions
.globl _start
_start:
    la      x1, test_data
    li      x2, 0x12345678
    sw      x2, 0(x1)
    lw      x3, 0(x1)
    li      x4, 0xABCD
    sh      x4, 4(x1)
    lh      x5, 4(x1)
    lhu     x6, 4(x1)
    li      x7, 0xEF
    sb      x7, 8(x1)
    lb      x8, 8(x1)
    lbu     x9, 8(x1)
    li      x10, 0xFFFFFFFF
    sw      x10, 12(x1)
    li      x11, 0x12
    sb      x11, 12(x1)
    lw      x12, 12(x1)
    li      a7, 93
    ecall
.data
.align 4
test_data:
    .space  32
