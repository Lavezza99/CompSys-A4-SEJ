# test_logic.s - Test logical operations
.globl _start
_start:
    li      x1, 0xFF
    li      x2, 0x0F
    and     x3, x1, x2
    li      x4, 0xF0
    li      x5, 0x0F
    or      x6, x4, x5
    li      x7, 0xFF
    li      x8, 0xF0
    xor     x9, x7, x8
    li      x10, 0xFF
    andi    x11, x10, 0x0F
    li      x12, 0xF0
    ori     x13, x12, 0x0F
    li      x14, 0xFF
    xori    x15, x14, 0xF0
    li      x16, 5
    li      x17, 10
    slt     x18, x16, x17
    slt     x19, x17, x16
    li      x20, 5
    li      x21, 10
    sltu    x22, x20, x21
    sltu    x23, x21, x20
    li      x24, 5
    slti    x25, x24, 10
    slti    x26, x24, 3
    li      x27, 5
    sltiu   x28, x27, 10
    sltiu   x29, x27, 3
    li      a7, 93
    ecall
