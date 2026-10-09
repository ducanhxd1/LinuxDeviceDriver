#!/bin/sh
set -eu
supervisor=${1:?"supervisor path required"}
worker=${2:?"worker path required"}
case "$supervisor" in /*) ;; *) supervisor="$(pwd)/${supervisor#./}" ;; esac
case "$worker" in /*) ;; *) worker="$(pwd)/${worker#./}" ;; esac
test_dir=$(mktemp -d)
supervisor_pid=
trap 'test -z "$supervisor_pid" || kill -KILL "$supervisor_pid" 2>/dev/null || true; rm -rf "$test_dir"' EXIT HUP INT TERM

"$supervisor" "$worker" > "$test_dir/run.log" 2>&1 &
supervisor_pid=$!
attempt=0
while ! grep -q '^supervisor: worker_pid=' "$test_dir/run.log"; do
    attempt=$((attempt + 1))
    test "$attempt" -lt 100 || exit 1
    sleep 0.02
done
worker_pid=$(sed -n 's/^supervisor: worker_pid=//p' "$test_dir/run.log" | head -n 1)
kill -TERM "$supervisor_pid"
wait "$supervisor_pid"
supervisor_pid=

grep -q '^supervisor: forwarding SIGTERM' "$test_dir/run.log"
grep -q "^worker pid=$worker_pid stopped" "$test_dir/run.log"
grep -q '^supervisor: worker exited code=0' "$test_dir/run.log"
if kill -0 "$worker_pid" 2>/dev/null; then exit 1; fi

if "$supervisor" "$test_dir/missing" > "$test_dir/exec.log" 2>&1; then exit 1; fi
grep -q 'worker exited code=127' "$test_dir/exec.log"
echo "PASS: process and signals"

