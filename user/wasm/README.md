# wasm — WebAssembly JIT for JOS

`wasm <file.wasm> [export] [i32/i64 args...]`

```
wasm.c       driver (umain), the plan is in comments there
common/      shared types, limits, opcodes, VA layout, arena allocator
parser/      bounds-checked reader, binary parser, validator
vm/          instance, linear memory, reference interpreter
vm/jit/      W^X code buffer, x86-64 encoder, baseline JIT
tests/       test modules: .c (clang --target=wasm32), .wat (wat2wasm if installed), .wasm
```

Build: `make wasm` (also part of the normal build and fs image).

Scope: Wasm 1.0 (MVP), integers only (i32/i64), one memory, one table,
function imports for host calls. Floats are rejected by the validator.

Spec: https://www.w3.org/TR/wasm-core-1/
