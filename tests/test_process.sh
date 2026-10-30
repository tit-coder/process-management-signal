#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_process.sh
# Description: Verifies process creation, worker execution, and normal exit.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/process_manager"

echo "=================================================="
echo " [TEST SUITE] test_process.sh: Process Lifecycle"
echo "=================================================="

# Ensure binary is built
if [ ! -f "$BINARY" ]; then
    echo "[BUILD] Building project..."
    make -C "$PROJECT_ROOT"
fi

echo "[STEP 1] Testing process start, worker counter, status check, and exit..."

# Input sequence:
# 3: Show status
# 5: Exit
OUTPUT=$(printf "3\n5\n" | "$BINARY")

echo "$OUTPUT"

echo "[VERIFICATION] Checking test criteria..."

# Check 1: Worker PID printed
if echo "$OUTPUT" | grep -q "Worker PID:"; then
    echo "  [PASS] Worker process was created and displayed PID."
else
    echo "  [FAIL] Worker PID not detected in output."
    exit 1
fi

# Check 2: Running output detected
if echo "$OUTPUT" | grep -q "Running..."; then
    echo "  [PASS] Worker loop is executing (Running... detected)."
else
    echo "  [FAIL] Worker loop execution not detected."
    exit 1
fi

# Check 3: Status is RUNNING
if echo "$OUTPUT" | grep -q "Status    : RUNNING"; then
    echo "  [PASS] Status is accurately reported as RUNNING."
else
    echo "  [FAIL] Status RUNNING not detected."
    exit 1
fi

# Check 4: Clean exit
if echo "$OUTPUT" | grep -q "Goodbye"; then
    echo "  [PASS] Process manager exited cleanly."
else
    echo "  [FAIL] Clean exit message not detected."
    exit 1
fi

echo "=================================================="
echo " [RESULT] test_process.sh: ALL CHECKS PASSED!"
echo "=================================================="
exit 0
