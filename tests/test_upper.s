# test_upper.s - Test LUI and AUIPC
.globl _start
_start:
    lui     x1, 0x12345
    lui     x2, 0xABCDE
    addi    x2, x2, 0x678
    auipc   x3, 0
    auipc   x4, 1
    auipc   x5, 0x10
    lui     x6, 0xDEADB
    addi    x6, x6, 0xEEF
    lui     x7, 0xFFFFF
    li      a7, 93
    ecall
