/* Integer arithmetic edge cases: wasm math.wasm <func> <args>
 * i64 functions take and return int, computing inside in 64 bits. */

typedef long long i64;
typedef unsigned long long u64;

int mul(int a, int b) { return a * b; }
int div_s(int a, int b) { return a / b; }
int rem_s(int a, int b) { return a % b; }
unsigned div_u(unsigned a, unsigned b) { return a / b; }
unsigned rem_u(unsigned a, unsigned b) { return a % b; }
int shl(int a, int b) { return a << (b & 31); }
int shr_s(int a, int b) { return a >> (b & 31); }
unsigned shr_u(unsigned a, unsigned b) { return a >> (b & 31); }
unsigned rotl(unsigned x, unsigned k) { k &= 31; return (x << k) | (x >> ((32 - k) & 31)); }
int clz(unsigned x) { return x ? __builtin_clz(x) : 32; }
int popcnt(unsigned x) { return __builtin_popcount(x); }
int lt_u(unsigned a, unsigned b) { return a < b; }

/* 64-bit: high/low halves of a product, signed division */
int mul64_hi(int a, int b) { return (int)(((i64)a * (i64)b) >> 32); }
int div64(int a, int b) { return (int)(((i64)a << 20) / (i64)b); }
int shr64_u(int a, int k) { return (int)(((u64)(unsigned)a << 32) >> (k & 63)); }
int clz64(int a) { u64 x = (u64)(unsigned)a; return x ? __builtin_clzll(x) : 64; }
