#!/usr/bin/env bash
# Build, flash, and monitor bunny-cam.
# Usage:
#   ./flash.sh              -- build, flash, then monitor
#   ./flash.sh build        -- build only
#   ./flash.sh flash        -- build + flash (no monitor)
#   ./flash.sh monitor      -- monitor only

set -e

IDF_PATH="C:/esp/v5.5.3/esp-idf"
PORT="COM3"
BAUD="115200"

# shellcheck disable=SC1091
. "$IDF_PATH/export.sh"

case "${1:-all}" in
    build)
        idf.py build
        ;;
    flash)
        idf.py -p "$PORT" build flash
        ;;
    monitor)
        idf.py -p "$PORT" -b "$BAUD" monitor
        ;;
    all)
        idf.py -p "$PORT" -b "$BAUD" build flash monitor
        ;;
    *)
        echo "Usage: $0 [build|flash|monitor|all]" >&2
        exit 1
        ;;
esac
