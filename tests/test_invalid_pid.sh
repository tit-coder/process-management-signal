#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_invalid_pid.sh
# Mo ta: Kiem tra xu ly loi doi voi PID khong hop le, PID khong ton tai,
#        va cac gia tri nhap menu khong hop le.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/process_manager"

echo "=================================================="
echo " [TEST SUITE] test_invalid_pid.sh: Error Handling"
echo "=================================================="

# Dam bao project da duoc bien dich
if [ ! -f "$BINARY" ]; then
    echo "[BUILD] Building project..."
    make -C "$PROJECT_ROOT"
fi

echo "[STEP 1] Testing invalid menu inputs (characters, out-of-range, negative numbers)..."

# Chuoi input kiem thu:
# "abc"       -> input chu cai khong hop le
# "-5"        -> so am khong hop le
# "99"        -> so ngoai dai menu khong hop le
# "" (empty)  -> chuoi rong / enter
# 5           -> Thoat chuong trinh
INPUT_SEQUENCE=$(cat <<EOF
abc
-5
99

5
EOF
)

OUTPUT=$(echo "$INPUT_SEQUENCE" | "$BINARY")

echo "$OUTPUT"

echo "[VERIFICATION] Checking invalid input handling..."

if echo "$OUTPUT" | grep -q "Invalid choice. Please enter a number between 1 and 5."; then
    echo "  [PASS] Invalid menu input correctly rejected with helpful message."
else
    echo "  [FAIL] Expected error message for invalid choice was not found."
    exit 1
fi

echo "[STEP 2] Compiling and running API-level Invalid PID unit test..."

TEST_INVALID_C="$PROJECT_ROOT/tests/temp_test_invalid_pid.c"
TEST_INVALID_BIN="$PROJECT_ROOT/tests/temp_test_invalid_pid"

# Dam bao file tam luon duoc don dep sach se ke ca khi co loi xay ra
trap 'rm -f "$TEST_INVALID_C" "$TEST_INVALID_BIN"' EXIT

cat <<'EOF' > "$TEST_INVALID_C"
#include <stdio.h>
#include <assert.h>
#include "../src/process_manager.h"
#include "../src/signal_handler.h"

int main(void) {
    /* Test 1: PID <= 0 */
    assert(is_process_alive(0) == 0);
    assert(is_process_alive(-1) == 0);
    assert(suspend_process(0) == -1);
    assert(suspend_process(-999) == -1);
    assert(resume_process(0) == -1);
    assert(resume_process(-999) == -1);
    assert(terminate_process(0) == -1);
    assert(terminate_process(-999) == -1);
    assert(wait_for_process(0) == -1);
    assert(wait_for_process(-1) == -1);

    /* Test 2: PID khong ton tai (vi du 9999999) */
    pid_t fake_pid = 9999999;
    assert(is_process_alive(fake_pid) == 0);
    assert(suspend_process(fake_pid) == -1);
    assert(resume_process(fake_pid) == -1);
    assert(terminate_process(fake_pid) == -1);

    printf("[PASS] All Invalid PID API assertions succeeded!\n");
    return 0;
}
EOF

gcc -Wall -Wextra -std=c11 \
    "$TEST_INVALID_C" \
    "$PROJECT_ROOT/src/process_manager.c" \
    "$PROJECT_ROOT/src/signal_handler.c" \
    "$PROJECT_ROOT/src/monitor.c" \
    -o "$TEST_INVALID_BIN"

"$TEST_INVALID_BIN"

echo "=================================================="
echo " [RESULT] test_invalid_pid.sh: ALL CHECKS PASSED!"
echo "=================================================="
exit 0

