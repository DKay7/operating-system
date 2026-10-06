/* Mandelbrot set in 16.16 fixed point: number of points of a size x size grid
 * over [-2, 1] x [-1.5, 1.5] that stay bounded for 64 iterations */

#define FIX(x) ((int)((x) * 65536))

int
mandel(int size) {
    int inside = 0;
    for (int py = 0; py < size; py++) {
        for (int px = 0; px < size; px++) {
            int cx = FIX(-2) + (int)((long long)FIX(3) * px / size);
            int cy = FIX(-1.5) + (int)((long long)FIX(3) * py / size);
            int x = 0, y = 0, i = 0;
            for (; i < 64; i++) {
                long long xx = (long long)x * x >> 16, yy = (long long)y * y >> 16;
                if (xx + yy > FIX(4))
                    break;
                y = (int)(((long long)x * y >> 15) + cy);
                x = (int)(xx - yy + cx);
            }
            inside += i == 64;
        }
    }
    return inside;
}
