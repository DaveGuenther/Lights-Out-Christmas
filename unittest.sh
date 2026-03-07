#!/usr/bin/env bash
# Run the Lights Out Christmas unit test suite.
# Works from any working directory; resolves paths relative to this script.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-Release"
TEST_EXE="$BUILD_DIR/bin/LightsOutTests.exe"

# ── Environment ──────────────────────────────────────────────────────────────
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH"

# ── Sanity checks ────────────────────────────────────────────────────────────
if [ ! -f "$TEST_EXE" ]; then
    echo "ERROR: Test executable not found: $TEST_EXE"
    echo "       Run cmake --build build-Release first."
    exit 1
fi

# ── Run tests ────────────────────────────────────────────────────────────────
echo "Running: $TEST_EXE"
echo ""
"$TEST_EXE" "$@"
