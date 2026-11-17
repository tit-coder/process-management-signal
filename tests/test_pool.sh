#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_pool.sh
# Purpose: Verify Multi-Worker Process Pool, Process Group (PGID), and Dynamic Spawning
# Author: TV2 & Linux System Programming Team
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${PROJECT_ROOT}/bin/process_control"

# ANSI Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}====================================================${NC}"
echo -e "${BLUE}   RUNNING TEST 4: Multi-Worker Process Pool        ${NC}"
echo -e "${BLUE}====================================================${NC}"

if [ ! -x "$BIN" ]; then
    echo -e "${RED}[FAIL] Executable $BIN not found or not executable. Run 'make' first.${NC}"
    exit 1
fi

TMP_OUT=$(mktemp)
cleanup() {
    rm -f "$TMP_OUT"
}
trap cleanup EXIT

echo -e "${YELLOW}[Step 1] Executing Process Pool verification suite...${NC}"
if "$BIN" --test-pool > "$TMP_OUT" 2>&1; then
    cat "$TMP_OUT"
    echo -e "${GREEN}[PASS] Multi-Worker Process Pool operations passed successfully.${NC}"
else
    echo -e "${RED}[FAIL] Process Pool operations test failed!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${YELLOW}[Step 2] Verifying no residual worker processes remain...${NC}"
sleep 0.5
if ps -ef | grep -v grep | grep -q "process_control.*worker"; then
    echo -e "${RED}[FAIL] Orphaned worker processes detected!${NC}"
    exit 1
else
    echo -e "${GREEN}[PASS] Clean teardown verified. Zero orphan/zombie workers in process table.${NC}"
fi

echo -e "${GREEN}>>> TEST 4 RESULT: ALL CHECKS PASSED <<<${NC}\n"
exit 0
