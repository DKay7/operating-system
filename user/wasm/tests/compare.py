#!/usr/bin/env python3
# Compare two bench.sh results: user/wasm/tests/compare.py interp.txt jit.txt
# Speedup = how much faster the second run is: (first / second - 1) * 100%

import sys


def load(path):
    cycles = {}
    for line in open(path):
        if "|" not in line:
            continue
        name, value = line.rsplit("|", 1)
        cycles[name.strip()] = int(value)
    return cycles


def main():
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} <first.txt> <second.txt>")

    first, second = load(sys.argv[1]), load(sys.argv[2])
    names = [name for name in first if name in second]

    width = max(len(name) for name in names + ["test"])
    print(f"{'test':<{width}} | {sys.argv[1]:>14} | {sys.argv[2]:>14} | {'speedup':>9}")
    print("-" * (width + 46))
    for name in names:
        speedup = (first[name] / second[name] - 1) * 100
        print(f"{name:<{width}} | {first[name]:>14} | {second[name]:>14} | {speedup:>+8.1f}%")

    for name in first.keys() ^ second.keys():
        print(f"{name}: only in one of the files")


if __name__ == "__main__":
    main()
