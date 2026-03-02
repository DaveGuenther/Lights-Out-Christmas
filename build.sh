#!/usr/bin/env bash
# build.sh — convenience build script for Lights Out Christmas
# Usage:
#   ./build.sh              # Release build
#   ./build.sh debug        # Debug build with ASan
#   ./build.sh test         # Build and run tests
#   ./build.sh clean        # Remove build directory

set -e

BUILD_TYPE="Release"
if [[ "$1" == "debug" ]]; then
    BUILD_TYPE="Debug"
fi

BUILD_DIR="build-${BUILD_TYPE,,}"

if [[ "$1" == "clean" ]]; then
    echo "Removing build directories..."
    rm -rf build-release build-debug
    exit 0
fi

echo "=== Lights Out Christmas ==="
echo "Build type: $BUILD_TYPE"
echo "Build dir:  $BUILD_DIR"
echo ""

cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
    -G "Ninja" \
    .

cmake --build "$BUILD_DIR" --parallel

if [[ "$1" == "test" ]]; then
    echo ""
    echo "=== Running tests ==="
    cd "$BUILD_DIR"
    ctest --output-on-failure
fi

echo ""
echo "=== Build complete ==="
echo "Executable: $BUILD_DIR/bin/LightsOutChristmas"
