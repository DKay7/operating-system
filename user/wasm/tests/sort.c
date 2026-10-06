/* Insertion sort of n pseudo-random numbers in an array on the wasm stack.
 * Returns a checksum of the sorted array, or -1 if it is not sorted. */

#define MAX 2048

int
sort(int n, int seed) {
    int a[MAX];
    if (n > MAX)
        n = MAX;

    unsigned x = seed;
    for (int i = 0; i < n; i++) {
        x = x * 1103515245 + 12345;
        a[i] = (x >> 16) & 0x7fff;
    }

    for (int i = 1; i < n; i++) {
        int key = a[i], j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }

    int checksum = 0;
    for (int i = 0; i < n; i++) {
        if (i > 0 && a[i - 1] > a[i])
            return -1;
        checksum = checksum * 31 + a[i];
    }
    return checksum;
}
