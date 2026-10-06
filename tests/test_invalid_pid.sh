#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_invalid_pid.sh
# Description: Verifies robust error handling against invalid PID values,
#              non-existent PIDs, and invalid menu choices.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/process_manager"

echo "=================================================="
echo " [TEST SUITE] test_invalid_pid.sh: Error Handling"
echo "=================================================="

# Ensure binary is built
if [ ! -f "$BINARY" ]; then
    echo "[BUILD] Building project..."
    make -C "$PROJECT_ROOT"
fi

echo "[STEP 1] Testing invalid menu inputs (characters, out-of-range, negative numbers)..."

# Input sequence:
# "abc"       -> invalid choice
# "-5"        -> invalid choice
# "99"        -> invalid choice
# "" (empty)  -> invalid choice
# 5           -> Exit cleanly
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

    /* Test 2: Non-existent PID (e.g. 9999999) */
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

# Clean up temporary test files
rm -f "$TEST_INVALID_C" "$TEST_INVALID_BIN"

echo "=================================================="
echo " [RESULT] test_invalid_pid.sh: ALL CHECKS PASSED!"
echo "=================================================="
exit 0
