static int
pick(int x, int acc) {
    switch (x) {
    case 0: return acc + 11;
    case 1: return acc * 3;
    case 2: return acc - 37;
    case 3: return acc ^ 41;
    case 4: return acc << 2;
    case 5: return acc / 7;
    case 7: return acc % 1000;
    default: return acc + x;
    }
}

int
switch_sum(int n) {
    int acc = 1;
    for (int i = -2; i < n; ++i)
        acc = pick(i, acc) + pick(i + 1, acc);
    return acc;
}
