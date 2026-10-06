/* Number of primes below n, by trial division */

static int
is_prime(int n) {
    if (n < 2)
        return 0;
    for (int d = 2; d * d <= n; d++)
        if (n % d == 0)
            return 0;
    return 1;
}

int
count_primes(int n) {
    int count = 0;
    for (int i = 0; i < n; i++)
        count += is_prime(i);
    return count;
}
