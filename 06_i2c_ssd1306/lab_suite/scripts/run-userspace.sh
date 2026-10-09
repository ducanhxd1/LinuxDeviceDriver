#!/bin/sh
set -eu
suite_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ "$#" -gt 1 ]; then
    echo "Usage: sh scripts/run-userspace.sh [/dev/i2c-1]" >&2
    exit 2
fi
exec "$suite_dir/build/oled-lab" --bus "${1:-/dev/i2c-1}" demo
