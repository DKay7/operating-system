#!/bin/bash
# Download everything doom.wasm needs into user/wasm/doom/third_party (not in git):
#   wasi-sysroot/           wasi-libc from wasi-sdk 22
#   libclang_rt.builtins-wasm32.a
#   doom-ascii/             doom-ascii sources with patches/*.patch applied
#   doom1.wad               shareware game data
set -euo pipefail

cd "$(dirname "$0")"
mkdir -p third_party
cd third_party

SDK=https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-22
WAD_DEB=http://deb.debian.org/debian/pool/non-free/d/doom-wad-shareware/doom-wad-shareware_1.9.fixed-5_all.deb
WAD_MD5=f0cefca49926d00903cf57551d901abe
DOOM_COMMIT=940528f138e12431e5a1417351863709f1620078

if [ ! -d wasi-sysroot ]; then
    curl -fsSL "$SDK/wasi-sysroot-22.0.tar.gz" | tar xz
fi

if [ ! -f libclang_rt.builtins-wasm32.a ]; then
    curl -fsSL "$SDK/libclang_rt.builtins-wasm32-wasi-22.0.tar.gz" | tar xz
    find . -path ./wasi-sysroot -prune -o -name libclang_rt.builtins-wasm32.a -print \
        | head -1 | xargs -I{} mv {} .
    rm -rf lib
fi

if [ ! -d doom-ascii ]; then
    curl -fsSL "https://github.com/wojciech-graj/doom-ascii/archive/$DOOM_COMMIT.tar.gz" | tar xz
    mv "doom-ascii-$DOOM_COMMIT" doom-ascii
    for p in ../patches/*.patch; do
        patch -s -p1 -d doom-ascii < "$p"
    done
fi

if [ ! -f doom1.wad ]; then
    tmp=$(mktemp -d)
    curl -fsSL "$WAD_DEB" -o "$tmp/wad.deb"
    (cd "$tmp" && ar x wad.deb && tar xf data.tar.*)
    find "$tmp" -iname doom1.wad -exec mv {} doom1.wad \;
    rm -rf "$tmp"
fi

echo "$WAD_MD5  doom1.wad" | md5sum -c --quiet
