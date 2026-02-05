#!/usr/bin/env bash
set -euo pipefail

# run_ccu.sh - helper to run ccu_daemon with correct CRSDK libs and env
# Usage:
#   SONY_PASS="Password1" ./run_ccu.sh [PORT]
#   export SONY_PASS="Password1"; ./run_ccu.sh 5555
# You can override CRSDK_ROOT by exporting it before running:
#   export CRSDK_ROOT=/path/to/CrSDK && SONY_PASS=... ./run_ccu.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

CRSDK_ROOT_DEFAULT="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8"
CRSDK_ROOT="${CRSDK_ROOT:-$CRSDK_ROOT_DEFAULT}"

export LD_LIBRARY_PATH="$CRSDK_ROOT/build:$CRSDK_ROOT/external/crsdk:$CRSDK_ROOT/external/crsdk/CrAdapter:$CRSDK_ROOT/external/opencv/Linux:${LD_LIBRARY_PATH:-}"

if [ "${1-}" = "--help" ] || [ "${1-}" = "-h" ]; then
  echo "run_ccu.sh - Start ccu_daemon with CRSDK runtime environment"
  echo
  echo "Usage: SONY_PASS=\"Password1\" ./run_ccu.sh [PORT]"
  echo "Default PORT=5555"
  echo "Options: --diag  Run a camera enumeration diagnostic and exit"
  exit 0
fi

if [ "${1-}" = "--diag" ]; then
  if [ ! -x "$BUILD_DIR/ccu_diag" ]; then
    echo "ERROR: $BUILD_DIR/ccu_diag not found. Build the project first: cmake --build ."
    exit 1
  fi
  echo "Running diagnostic: $BUILD_DIR/ccu_diag"
  "$BUILD_DIR/ccu_diag"
  exit $? 
fi

if [ -z "${SONY_PASS-}" ]; then
  echo "ERROR: SONY_PASS not set. Set it in the environment first."
  echo "Example: SONY_PASS=\"Password1\" ./run_ccu.sh"
  exit 1
fi

PORT="${1:-5555}"
DAEMON="$BUILD_DIR/ccu_daemon"
CLI="$BUILD_DIR/ccu_cli"

if [ ! -x "$DAEMON" ]; then
  echo "ERROR: $DAEMON not found or not executable. Build the project first: cmake --build ."
  exit 1
fi

echo "Starting ccu_daemon on port $PORT"
echo "Using CRSDK_ROOT=$CRSDK_ROOT"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

exec "$DAEMON" "$PORT"
