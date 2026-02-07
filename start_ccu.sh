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
  any_pass=""
  for i in 0 1 2 3 4 5 6 7; do
    v="SONY_PASS_${i}"
    if [ -n "${!v-}" ]; then
      any_pass="1"
      break
    fi
  done
  if [ -z "$any_pass" ]; then
    echo "ERROR: SONY_PASS not set (and no SONY_PASS_0..7 found).\nSet it inline or export it before running the script.\nExamples:\n  SONY_PASS=\"Password1\" ./start_ccu.sh\n  export SONY_PASS=\"Password1\"; ./start_ccu.sh\n  SONY_PASS_0=\"Password1\" SONY_CAMERA_IP_0=\"192.168.0.70\" ./start_ccu.sh"
    exit 1
  fi
fi

# Forward all args to underlying runner
exec "$RUNNER" "$@"
