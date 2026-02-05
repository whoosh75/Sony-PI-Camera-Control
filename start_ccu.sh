#!/usr/bin/env bash
set -euo pipefail

# start_ccu.sh - convenience wrapper to run the CCU daemon or diagnostic quickly
# Usage (recommended):
#   SONY_PASS="Password1" ./start_ccu.sh         # start daemon on default port 5555
#   SONY_PASS="Password1" ./start_ccu.sh --diag  # run enumeration diagnostic
#   SONY_PASS="Password1" SONY_CAMERA_IP="192.168.33.94" ./start_ccu.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PI_DIR="$SCRIPT_DIR/pi_controller"
RUNNER="$PI_DIR/run_ccu.sh"

if [ ! -x "$RUNNER" ]; then
  echo "ERROR: helper $RUNNER not found or not executable. Build the project and try again."
  exit 1
fi

if [ -z "${SONY_PASS-}" ]; then
  echo "ERROR: SONY_PASS not set.\nSet it inline or export it before running the script.\nExamples:\n  SONY_PASS=\"Password1\" ./start_ccu.sh\n  export SONY_PASS=\"Password1\"; ./start_ccu.sh"
  exit 1
fi

# Forward all args to underlying runner
exec "$RUNNER" "$@"
