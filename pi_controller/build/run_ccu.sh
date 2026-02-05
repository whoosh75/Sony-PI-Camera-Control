#!/usr/bin/env bash
# Small wrapper in build/ so you can run ./run_ccu.sh from the build directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SCRIPT_DIR/../run_ccu.sh" "$@"
