#!/usr/bin/env bash
# Top-level convenience wrapper for running the CCU daemon/diag from repo root.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PI_DIR="$SCRIPT_DIR/pi_controller"
if [ ! -x "$PI_DIR/run_ccu.sh" ]; then
  echo "ERROR: $PI_DIR/run_ccu.sh not found or not executable. Please ensure you built the project and that the file exists."
  exit 1
fi
exec "$PI_DIR/run_ccu.sh" "$@"
