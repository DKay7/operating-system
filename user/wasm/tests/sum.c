/* Uses only local.get, i32.add, call and end: wasm sum.wasm sum3 1 2 3 */

__attribute__((noinline)) int
sum2(int a, int b) {
    return a + b;
}

int
sum3(int a, int b, int c) {
    return sum2(sum2(a, b), c);
}
