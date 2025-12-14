#!/bin/bash
# Simple script to run all RISC-V tests
# Run from tests/ directory

echo "Running RISC-V tests..."

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

mkdir -p logs

PASSED=0
FAILED=0

for test in test_*.elf; do
    if [ -f "$test" ]; then
        base="${test%.elf}"
        echo -n "Testing $base... "
        
        if ../sim "$test" -s "logs/$base.log" > "logs/$base.out" 2>&1; then
            echo -e "${GREEN}✓ PASSED${NC}"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}✗ FAILED${NC}"
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo ""
echo "========================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "========================================"