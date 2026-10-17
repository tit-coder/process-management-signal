#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_process.sh
# Mo ta: Kiem tra khoi tao tien trinh, vong lap worker, va thoat chuong trinh.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/process_manager"

echo "=================================================="
echo " [TEST SUITE] test_process.sh: Process Lifecycle"
echo "=================================================="

# Dam bao project da duoc bien dich
if [ ! -f "$BINARY" ]; then
    echo "[BUILD] Building project..."
    make -C "$PROJECT_ROOT"
fi

echo "[STEP 1] Testing process start, worker counter, status check, and exit..."

# Chuoi thao tac nhap:
# 3: Show status (Kiem tra trang thai)
# 5: Exit (Thoat)
OUTPUT=$(printf "3\n5\n" | "$BINARY")

echo "$OUTPUT"

echo "[VERIFICATION] Checking test criteria..."

# Kiem tra 1: In ra Worker PID
if echo "$OUTPUT" | grep -q "Worker PID:"; then
    echo "  [PASS] Worker process was created and displayed PID."
else
    echo "  [FAIL] Worker PID not detected in output."
    exit 1
fi

# Kiem tra 2: Phat hien worker dang chay
if echo "$OUTPUT" | grep -q "Running..."; then
    echo "  [PASS] Worker loop is executing (Running... detected)."
else
    echo "  [FAIL] Worker loop execution not detected."
    exit 1
fi

# Kiem tra 3: Trang thai chinh xac la RUNNING
if echo "$OUTPUT" | grep -q "Status    : RUNNING"; then
    echo "  [PASS] Status is accurately reported as RUNNING."
else
    echo "  [FAIL] Status RUNNING not detected."
    exit 1
fi

# Kiem tra 4: Thoat sach se
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

