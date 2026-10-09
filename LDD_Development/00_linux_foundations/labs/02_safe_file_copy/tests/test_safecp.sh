#!/bin/sh
set -eu
program=${1:?"binary path required"}
case "$program" in /*) ;; *) program="$(pwd)/${program#./}" ;; esac
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cd "$test_dir"

: > empty
"$program" empty empty.copy
cmp empty empty.copy
printf 'hello\nLinux\n' > text
"$program" text text.copy
cmp text text.copy
dd if=/dev/urandom of=binary bs=1024 count=80 status=none
"$program" binary binary.copy
cmp binary binary.copy
if "$program" text text.copy 2>/dev/null; then exit 1; fi
"$program" --force text text.copy
ln text same-inode
if "$program" --force text same-inode 2>/dev/null; then exit 1; fi
if "$program" missing nowhere 2>/dev/null; then exit 1; fi
echo "PASS: safecp"

