#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_scheduler.sh
# Purpose: Verify User-Space Preemptive Round-Robin Scheduler (SIGSTOP/SIGCONT/SIGALRM)
# Author: Leader & Linux System Programming Team
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
echo -e "${BLUE}   RUNNING TEST 5: Preemptive Round-Robin Scheduler ${NC}"
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

echo -e "${YELLOW}[Step 1] Executing Preemptive Scheduler verification suite...${NC}"
if "$BIN" --test-scheduler > "$TMP_OUT" 2>&1; then
    cat "$TMP_OUT"
    echo -e "${GREEN}[PASS] Preemptive Round-Robin context switching verified.${NC}"
else
    echo -e "${RED}[FAIL] Preemptive scheduler test failed!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${YELLOW}[Step 2] Verifying Scheduler Audit Trail...${NC}"
if grep -q "SCHED_PREEMPT" "${PROJECT_ROOT}/logs/audit.log" && grep -q "SCHED_DISPATCH" "${PROJECT_ROOT}/logs/audit.log"; then
    echo -e "${GREEN}[PASS] Audit trail confirms SCHED_PREEMPT (SIGSTOP) and SCHED_DISPATCH (SIGCONT).${NC}"
else
    echo -e "${RED}[FAIL] Audit log missing scheduler context switch entries!${NC}"
    exit 1
fi

echo -e "${GREEN}>>> TEST 5 RESULT: ALL CHECKS PASSED <<<${NC}\n"
exit 0
