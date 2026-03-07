#!/usr/bin/env bash
# launch.sh — run the Lights Out Christmas executable
# Usage:
#   ./launch.sh           # Run Release build (default)
#   ./launch.sh debug     # Run Debug build

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_TYPE="Release"
if [[ "$1" == "debug" ]]; then
    BUILD_TYPE="Debug"
fi

EXE="$SCRIPT_DIR/build-${BUILD_TYPE,,}/bin/LightsOutChristmas.exe"

if [ ! -f "$EXE" ]; then
    echo "ERROR: Executable not found: $EXE"
    echo "       Run ./build.sh first."
    exit 1
fi

exec "$EXE"
