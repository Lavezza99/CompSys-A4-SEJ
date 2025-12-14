#!/bin/bash
# Simple script to build all RISC-V tests
# Run from tests/ directory

echo "Building RISC-V tests..."

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

BUILT=0
FAILED=0

for test in test_*.s; do
    if [ -f "$test" ]; then
        base="${test%.s}"
        echo -n "Building $base... "
        
        if riscv32-unknown-elf-as -march=rv32im -mabi=ilp32 "$test" -o "$base.o" 2>/dev/null && \
           riscv32-unknown-elf-ld -m elf32lriscv -T link.ld "$base.o" -o "$base.elf" 2>/dev/null; then
            echo -e "${GREEN}✓${NC}"
            BUILT=$((BUILT + 1))
            rm "$base.o"  # Clean up .o file
        else
            echo -e "${RED}✗${NC}"
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo ""
echo "Built: $BUILT"
echo "Failed: $FAILED"