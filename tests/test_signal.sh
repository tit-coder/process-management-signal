#!/usr/bin/env bash
# ==============================================================================
# Test Script: test_signal.sh
# Mo ta: Kiem tra SIGSTOP (Suspend), SIGCONT (Resume), SIGTERM (Terminate),
#        va tinh bat bien khi gui lap tin hieu nhieu lan.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_ROOT/process_manager"

echo "=================================================="
echo " [TEST SUITE] test_signal.sh: Signal Controls"
echo "=================================================="

# Dam bao project da duoc bien dich
if [ ! -f "$BINARY" ]; then
    echo "[BUILD] Building project..."
    make -C "$PROJECT_ROOT"
fi

echo "[STEP 1] Running sequence: Suspend -> Suspend again -> Status -> Resume -> Resume again -> Status -> Terminate -> Terminate again -> Exit..."

# Chuoi thao tac nhap:
# 1: Suspend (Tam dung)
# 1: Suspend again (Tam dung lan nua)
# 3: Show status (Kiem tra status STOPPED)
# 2: Resume (Tiep tuc)
# 2: Resume again (Tiep tuc lan nua)
# 3: Show status (Kiem tra status RUNNING)
# 4: Terminate (Ket thuc)
# 4: Terminate again (Ket thuc lai de kiem tra xu ly an toan)
# 5: Exit (Thoat)
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

# Kiem tra 1: Gui SIGSTOP thanh cong
if echo "$OUTPUT" | grep -q "Process .* suspended (SIGSTOP sent)"; then
    echo "  [PASS] SIGSTOP successfully sent and acknowledged."
else
    echo "  [FAIL] SIGSTOP acknowledgment not found."
    exit 1
fi

# Kiem tra 2: Trang thai chuyen thanh STOPPED
if echo "$OUTPUT" | grep -q "Status    : STOPPED"; then
    echo "  [PASS] Process state changed to STOPPED."
else
    echo "  [FAIL] State STOPPED not detected."
    exit 1
fi

# Kiem tra 3: Gui SIGCONT thanh cong
if echo "$OUTPUT" | grep -q "Process .* resumed (SIGCONT sent)"; then
    echo "  [PASS] SIGCONT successfully sent and acknowledged."
else
    echo "  [FAIL] SIGCONT acknowledgment not found."
    exit 1
fi

# Kiem tra 4: Trang thai khoi phuc RUNNING
if echo "$OUTPUT" | grep -q "Status    : RUNNING"; then
    echo "  [PASS] Process state successfully resumed to RUNNING."
else
    echo "  [FAIL] State RUNNING not detected after resume."
    exit 1
fi

# Kiem tra 5: Gui SIGTERM ket thuc thanh cong
if echo "$OUTPUT" | grep -q "Process .* terminated (SIGTERM sent)"; then
    echo "  [PASS] SIGTERM successfully sent and acknowledged."
else
    echo "  [FAIL] SIGTERM acknowledgment not found."
    exit 1
fi

# Kiem tra 6: Xu ly an toan khi terminate lai tien trinh da chet
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

