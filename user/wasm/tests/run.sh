echo fib 20, expected 6765
wasm fib.wasm fib 20

echo fib 25, expected 75025
wasm fib.wasm fib 25

echo sum3 1 2 39, expected 42
wasm sum.wasm sum3 1 2 39

echo div_s -7 2, expected -3
wasm math.wasm div_s -7 2

echo mul64_hi 65536 65536, expected 1
wasm math.wasm mul64_hi 65536 65536

echo clz64 1, expected 63
wasm math.wasm clz64 1

echo sum_to 1000, expected 500500
wasm loops.wasm sum_to 1000

echo collatz 27, expected 111
wasm loops.wasm collatz 27

echo gcd 1071 462, expected 21
wasm loops.wasm gcd 1071 462

echo pow_mod 2 1000 1000003, expected 510646
wasm loops.wasm pow_mod 2 1000 1000003

echo count_primes 10000, expected 1229
wasm primes.wasm count_primes 10000

echo sort 1000 42, expected -2057836567
wasm sort.wasm sort 1000 42

echo mandel 32, expected 183
wasm mandel.wasm mandel 32

echo wasm tests done
