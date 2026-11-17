#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_signal.sh
# Purpose: Verify POSIX Signal Control (SIGSTOP, SIGCONT, SIGTERM) and Audit Logging
# Author: TV3 & Linux System Programming Team
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
echo -e "${BLUE}   RUNNING TEST 2: Signal Control (SIGSTOP/CONT/TERM)${NC}"
echo -e "${BLUE}====================================================${NC}"

if [ ! -x "$BIN" ]; then
    echo -e "${RED}[FAIL] Executable $BIN not found or not executable. Run 'make' first.${NC}"
    exit 1
fi

TMP_OUT=$(mktemp)
CLEANUP_PIDS=()
cleanup() {
    rm -f "$TMP_OUT"
    for p in "${CLEANUP_PIDS[@]}"; do
        if kill -0 "$p" 2>/dev/null; then
            kill -9 "$p" 2>/dev/null || true
        fi
    done
}
trap cleanup EXIT

# Spawn a dedicated background test process
sleep 30 &
TARGET_PID=$!
CLEANUP_PIDS+=("$TARGET_PID")

echo -e "${YELLOW}[Step 1] Testing SIGSTOP dispatching...${NC}"
if "$BIN" --test-signal SIGSTOP "$TARGET_PID" > "$TMP_OUT" 2>&1; then
    STATE=$(awk '/State:/ {print $2}' "/proc/${TARGET_PID}/status" 2>/dev/null || echo "UNKNOWN")
    if [[ "$STATE" == "T" || "$STATE" == "t" ]]; then
        echo -e "${GREEN}[PASS] Target PID ${TARGET_PID} successfully suspended (State: ${STATE}).${NC}"
    else
        echo -e "${RED}[FAIL] Target PID state is '${STATE}', expected 'T'!${NC}"
        cat "$TMP_OUT"
        exit 1
    fi
else
    echo -e "${RED}[FAIL] Failed to dispatch SIGSTOP!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${YELLOW}[Step 2] Testing SIGCONT dispatching...${NC}"
if "$BIN" --test-signal SIGCONT "$TARGET_PID" > "$TMP_OUT" 2>&1; then
    STATE=$(awk '/State:/ {print $2}' "/proc/${TARGET_PID}/status" 2>/dev/null || echo "UNKNOWN")
    if [[ "$STATE" == "S" || "$STATE" == "R" ]]; then
        echo -e "${GREEN}[PASS] Target PID ${TARGET_PID} successfully resumed (State: ${STATE}).${NC}"
    else
        echo -e "${RED}[FAIL] Target PID state is '${STATE}', expected 'S' or 'R'!${NC}"
        cat "$TMP_OUT"
        exit 1
    fi
else
    echo -e "${RED}[FAIL] Failed to dispatch SIGCONT!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${YELLOW}[Step 3] Testing status monitor on active PID...${NC}"
if "$BIN" --test-status "$TARGET_PID" > "$TMP_OUT" 2>&1; then
    if grep -q "PROCESS STATUS (/proc/${TARGET_PID}/status)" "$TMP_OUT"; then
        echo -e "${GREEN}[PASS] Process status correctly read and parsed from /proc.${NC}"
    else
        echo -e "${RED}[FAIL] Status monitor table header missing!${NC}"
        cat "$TMP_OUT"
        exit 1
    fi
else
    echo -e "${RED}[FAIL] Status monitor check failed!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${YELLOW}[Step 4] Testing SIGTERM termination...${NC}"
if "$BIN" --test-signal SIGTERM "$TARGET_PID" > "$TMP_OUT" 2>&1; then
    sleep 0.2
    if kill -0 "$TARGET_PID" 2>/dev/null; then
        echo -e "${RED}[FAIL] Target PID ${TARGET_PID} still alive after SIGTERM!${NC}"
        exit 1
    else
        echo -e "${GREEN}[PASS] Target PID ${TARGET_PID} successfully terminated by SIGTERM.${NC}"
    fi
else
    echo -e "${RED}[FAIL] Failed to dispatch SIGTERM!${NC}"
    cat "$TMP_OUT"
    exit 1
fi

echo -e "${YELLOW}[Step 5] Verifying Audit Trail in logs/audit.log...${NC}"
if [ -f "${PROJECT_ROOT}/logs/audit.log" ]; then
    if grep -q "SIGSTOP" "${PROJECT_ROOT}/logs/audit.log" && grep -q "SIGCONT" "${PROJECT_ROOT}/logs/audit.log"; then
        echo -e "${GREEN}[PASS] Audit log entries verified in logs/audit.log.${NC}"
    else
        echo -e "${RED}[FAIL] Expected audit entries missing in logs/audit.log!${NC}"
        exit 1
    fi
else
    echo -e "${RED}[FAIL] logs/audit.log not found!${NC}"
    exit 1
fi

echo -e "${GREEN}>>> TEST 2 RESULT: ALL CHECKS PASSED <<<${NC}\n"
exit 0
