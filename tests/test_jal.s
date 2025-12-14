# test_jal.s - Test JAL and JALR
.globl _start
_start:
    jal     ra, func1
    li      x1, 1
    la      x2, func2
    jalr    ra, x2, 0
    li      x3, 2
    li      a7, 93
    ecall
func1:
    li      x10, 100
    jalr    x0, ra, 0
func2:
    li      x11, 200
    jalr    x0, ra, 0
