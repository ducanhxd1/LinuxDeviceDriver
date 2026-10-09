#!/bin/sh
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
module="$script_dir/../build/ldd_hello.ko"
name=ldd_hello

test "$(id -u)" -eq 0 || { echo "run with sudo" >&2; exit 1; }
test -f "$module" || { echo "build module first" >&2; exit 1; }
test "$(modinfo -F vermagic "$module" | awk '{print $1}')" = "$(uname -r)" || {
    echo "module vermagic does not match running kernel" >&2; exit 1;
}
test ! -d "/sys/module/$name" || { echo "$name is already loaded" >&2; exit 1; }

loaded=0
cleanup() {
    if test "$loaded" -eq 1 && test -d "/sys/module/$name"; then
        rmmod "$name"
    fi
}
trap cleanup EXIT HUP INT TERM

insmod "$module"
loaded=1
test -d "/sys/module/$name"
grep -w "$name" /proc/modules
rmmod "$name"
loaded=0
test ! -d "/sys/module/$name"
echo "PASS: load and unload; inspect kernel log for init/exit records"

