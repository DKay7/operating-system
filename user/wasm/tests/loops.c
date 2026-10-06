
int
sum_to(int n) {
    int sum = 0;
    for (int i = 1; i <= n; i++)
        sum += i;
    return sum;
}

int
collatz(int n) {
    int steps = 0;
    while (n != 1) {
        n = (n & 1) ? 3 * n + 1 : n / 2;
        steps++;
    }
    return steps;
}

int
gcd(int a, int b) {
    while (b) {
        int t = a % b;
        a = b;
        b = t;
    }
    return a;
}

int
pow_mod(int b, int e, int m) {
    long long result = 1, base = b % m;
    while (e > 0) {
        if (e & 1)
            result = result * base % m;
        base = base * base % m;
        e >>= 1;
    }
    return (int)result;
}
