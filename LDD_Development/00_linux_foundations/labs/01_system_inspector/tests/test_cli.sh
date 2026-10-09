#!/bin/sh
set -eu

program=${1:?"binary path is required"}
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

"$program" uptime > "$tmp_dir/uptime.out"
grep -Eq '^uptime_seconds=[0-9]+\.[0-9]{2}$' "$tmp_dir/uptime.out"

"$program" memory > "$tmp_dir/memory.out"
grep -Eq '^memory_total_kib=[0-9]+$' "$tmp_dir/memory.out"
grep -Eq '^memory_available_kib=[0-9]+$' "$tmp_dir/memory.out"

"$program" all > "$tmp_dir/all.out"
grep -q '^uptime_seconds=' "$tmp_dir/all.out"
grep -q '^memory_total_kib=' "$tmp_dir/all.out"

if "$program" invalid > "$tmp_dir/invalid.out" 2> "$tmp_dir/invalid.err"; then
    echo "invalid command unexpectedly succeeded" >&2
    exit 1
fi
grep -q '^unknown command:' "$tmp_dir/invalid.err"
grep -q '^Usage:' "$tmp_dir/invalid.err"

echo "PASS: system inspector CLI"

