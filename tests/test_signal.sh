#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_signal.sh
# Description: Verifies SIGSTOP (Suspend), SIGCONT (Resume), SIGTERM (Terminate),
#              and multiple/idempotent signal operations.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/process_manager"

echo "=================================================="
echo " [TEST SUITE] test_signal.sh: Signal Controls"
echo "=================================================="

# Ensure binary is built
if [ ! -f "$BINARY" ]; then
    echo "[BUILD] Building project..."
    make -C "$PROJECT_ROOT"
fi

echo "[STEP 1] Running sequence: Suspend -> Suspend again -> Status -> Resume -> Resume again -> Status -> Terminate -> Terminate again -> Exit..."

# Input sequence:
# 1: Suspend
# 1: Suspend again
# 3: Show status (expected STOPPED)
# 2: Resume
# 2: Resume again
# 3: Show status (expected RUNNING)
# 4: Terminate
# 4: Terminate again (handled gracefully)
# 5: Exit
INPUT_SEQUENCE=$(cat <<EOF
1
1
3
2
2
3
4
4
5
EOF
)

OUTPUT=$(echo "$INPUT_SEQUENCE" | "$BINARY")

echo "$OUTPUT"

echo "[VERIFICATION] Checking test criteria..."

# Check 1: Suspend success
if echo "$OUTPUT" | grep -q "Process .* suspended (SIGSTOP sent)"; then
    echo "  [PASS] SIGSTOP successfully sent and acknowledged."
else
    echo "  [FAIL] SIGSTOP acknowledgment not found."
    exit 1
fi

# Check 2: Stopped status detected
if echo "$OUTPUT" | grep -q "Status    : STOPPED"; then
    echo "  [PASS] Process state changed to STOPPED."
else
    echo "  [FAIL] State STOPPED not detected."
    exit 1
fi

# Check 3: Resume success
if echo "$OUTPUT" | grep -q "Process .* resumed (SIGCONT sent)"; then
    echo "  [PASS] SIGCONT successfully sent and acknowledged."
else
    echo "  [FAIL] SIGCONT acknowledgment not found."
    exit 1
fi

# Check 4: Running status restored
if echo "$OUTPUT" | grep -q "Status    : RUNNING"; then
    echo "  [PASS] Process state successfully resumed to RUNNING."
else
    echo "  [FAIL] State RUNNING not detected after resume."
    exit 1
fi

# Check 5: Terminate success
if echo "$OUTPUT" | grep -q "Process .* terminated (SIGTERM sent)"; then
    echo "  [PASS] SIGTERM successfully sent and acknowledged."
else
    echo "  [FAIL] SIGTERM acknowledgment not found."
    exit 1
fi

# Check 6: Repeated terminate handled
if echo "$OUTPUT" | grep -q "already terminated"; then
    echo "  [PASS] Repeated terminate correctly identified terminated process."
else
    echo "  [FAIL] Handling of terminated process on repeated termination failed."
    exit 1
fi

echo "=================================================="
echo " [RESULT] test_signal.sh: ALL CHECKS PASSED!"
echo "=================================================="
exit 0
