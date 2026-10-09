int
select_mix(int n) {
    int acc = 0;
    for (int i = -n; i < n; ++i) {
        int a = i * 7 + acc;
        int b = acc - i * 3;
        acc ^= (i & 1) ? a : b;
        acc += (acc > i) ? i : -i;
    }
    return acc;
}
