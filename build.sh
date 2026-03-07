#!/usr/bin/env bash
# build.sh — convenience build script for Lights Out Christmas
# Usage:
#   ./build.sh                # Release build
#   ./build.sh debug          # Debug build with ASan
#   ./build.sh test           # Build and run tests
#   ./build.sh clean          # Remove build directories
#   ./build.sh force-rebuild  # Touch main.cpp to force relink + asset copy

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ── Environment ──────────────────────────────────────────────────────────────
# Prefer MSYS2 mingw64 toolchain on Windows; no-op on Linux/Mac where these
# paths won't exist.
for MSYS_ROOT in "$MSYS2_ROOT/mingw64" "C:/msys64/mingw64" "C:/msys2/mingw64"; do
    if [ -d "$MSYS_ROOT/bin" ]; then
        export PATH="$MSYS_ROOT/bin:/c/msys64/usr/bin:$PATH"
        CMAKE="$MSYS_ROOT/bin/cmake.exe"
        NINJA="$MSYS_ROOT/bin/ninja.exe"
        CXX="$MSYS_ROOT/bin/clang++.exe"
        break
    fi
done

# Fall back to whatever is on PATH if not on Windows / MSYS2
CMAKE="${CMAKE:-cmake}"
NINJA="${NINJA:-ninja}"
CXX="${CXX:-clang++}"

# ── Arguments ────────────────────────────────────────────────────────────────
BUILD_TYPE="Release"
if [[ "$1" == "debug" ]]; then
    BUILD_TYPE="Debug"
fi

BUILD_DIR="$SCRIPT_DIR/build-${BUILD_TYPE,,}"

if [[ "$1" == "clean" ]]; then
    echo "Removing build directories..."
    rm -rf "$SCRIPT_DIR/build-release" "$SCRIPT_DIR/build-debug"
    exit 0
fi

if [[ "$1" == "force-rebuild" ]]; then
    echo "Forcing relink + asset copy by touching src/main.cpp..."
    touch "$SCRIPT_DIR/src/main.cpp"
fi

echo "=== Lights Out Christmas ==="
echo "Build type: $BUILD_TYPE"
echo "Build dir:  $BUILD_DIR"
echo "CMake:      $CMAKE"
echo "Ninja:      $NINJA"
echo "CXX:        $CXX"
echo ""

"$CMAKE" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_MAKE_PROGRAM="$NINJA" \
    -G "Ninja" \
    "$SCRIPT_DIR"

"$CMAKE" --build "$BUILD_DIR" --parallel

if [[ "$1" == "test" ]]; then
    echo ""
    echo "=== Running tests ==="
    "$BUILD_DIR/bin/LightsOutTests.exe" 2>/dev/null \
        || "$BUILD_DIR/bin/LightsOutTests"
fi

echo ""
echo "=== Build complete ==="
echo "Executable: $BUILD_DIR/bin/LightsOutChristmas"
