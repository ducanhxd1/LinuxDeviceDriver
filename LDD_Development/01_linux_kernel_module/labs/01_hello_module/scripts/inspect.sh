#!/bin/sh
set -eu
module=${1:?"module path required"}
test -f "$module"
test "$(modinfo -F name "$module")" = "ldd_hello"
test "$(modinfo -F license "$module")" = "GPL"
modinfo -F vermagic "$module" | grep -q "^$(uname -r) "
nm "$module" | grep -q 'ldd_hello_init'
nm "$module" | grep -q 'ldd_hello_exit'
echo "PASS: module metadata and symbols"

