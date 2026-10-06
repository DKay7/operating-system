#!/bin/bash
# Boot JOS in QEMU, run tests/run.sh there and save a table to a host file:
#   test | expected | got | cycles
# Usage (from the jos root): user/wasm/tests/bench.sh results.txt
# Compare two runs: paste interp.txt jit.txt

out=${1:-bench.txt}
dir=$(mktemp -d)
mkfifo "$dir/in"

make qemu-nox <"$dir/in" >"$dir/log" 2>&1 &
exec 3>"$dir/in"

wait_for() {
    until grep -q "$1" "$dir/log"; do sleep 1; done
}

wait_for '^\$ '                  # shell prompt
printf 'sh run.sh\r' >&3
wait_for 'wasm tests done'
printf '\001x' >&3               # Ctrl-A x: quit QEMU
wait

# "fib 20, expected 6765" / "fib = 6765" / "cycles: N" -> one row
tr -d '\r' <"$dir/log" | awk '
    /, expected / { split($0, part, ", expected "); name = part[1]; expected = part[2]; got = "-" }
    / = /         { got = $3 }
    /^cycles: /   { printf "%-24s | %-12s | %-12s | %s\n", name, $2 }
' >"$out"

rm -r "$dir"
cat "$out"
