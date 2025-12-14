#!/bin/bash
# This script creates ALL test files automatically
# Run from tests/ directory

echo "Creating all test files..."

# test_add.s
cat > test_add.s << 'EOF'
# test_add.s - Test ADD instruction
.globl _start
_start:
    li      x1, 10
    li      x2, 20
    add     x3, x1, x2
    li      x4, 0
    add     x5, x3, x4
    li      x6, -1
    li      x7, 1
    add     x8, x6, x7
    li      a7, 93
    ecall
EOF

# test_sub.s
cat > test_sub.s << 'EOF'
# test_sub.s - Test SUB instruction
.globl _start
_start:
    li      x1, 50
    li      x2, 20
    sub     x3, x1, x2
    li      x4, 10
    li      x5, 20
    sub     x6, x4, x5
    li      x7, 0
    li      x8, 5
    sub     x9, x7, x8
    li      a7, 93
    ecall
EOF

# test_mul.s
cat > test_mul.s << 'EOF'
# test_mul.s - Test MUL instruction
.globl _start
_start:
    li      x1, 10
    li      x2, 20
    mul     x3, x1, x2
    li      x4, 100
    li      x5, 0
    mul     x6, x4, x5
    li      x7, 42
    li      x8, 1
    mul     x9, x7, x8
    li      x10, -5
    li      x11, 3
    mul     x12, x10, x11
    li      a7, 93
    ecall
EOF

# test_div.s
cat > test_div.s << 'EOF'
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
EOF

# test_beq.s
cat > test_beq.s << 'EOF'
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
EOF

# test_lw_sw.s
cat > test_lw_sw.s << 'EOF'
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
EOF

# test_jal.s
cat > test_jal.s << 'EOF'
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
EOF

# test_shifts.s
cat > test_shifts.s << 'EOF'
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
EOF

# test_logic.s
cat > test_logic.s << 'EOF'
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
EOF

# test_branches.s
cat > test_branches.s << 'EOF'
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
EOF

# test_loads.s
cat > test_loads.s << 'EOF'
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
EOF

# test_stores.s
cat > test_stores.s << 'EOF'
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
EOF

# test_upper.s
cat > test_upper.s << 'EOF'
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
EOF

echo "✓ All test files created!"
echo ""
echo "Files created:"
ls -1 test_*.s
echo ""
echo "Now run: make all"