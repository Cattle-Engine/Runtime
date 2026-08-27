# namespace-refactor

A small clang LibTooling program for renaming a C++ namespace, and every
reference to it, across a project driven by `compile_commands.json`.

It handles:

- `namespace Old { ... }` (including C++17 `namespace Old::Sub { ... }`)
- `using namespace Old;`
- `using Old::Thing;`
- `namespace Alias = Old;`
- Qualified name usage: `Old::foo()`, `Old::Type value;`, `Old::Sub::Thing`, etc.

It does **not** rewrite `// namespace Old` closing-brace comments, and it
skips any occurrence that only exists inside a macro expansion (rewriting
those safely would require rewriting the macro definition itself).

## Build

You need LLVM/Clang development files
```bash
# Debian/Ubuntu
sudo apt install llvm-dev libclang-dev clang cmake ninja-build
```

Then:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

If `find_package(Clang)` can't locate the install, point CMake at it
explicitly, e.g. `-DClang_DIR=/usr/lib/llvm-18/lib/cmake/clang`.

## Usage

```bash
./build/namespace-refactor \
  -old=OldNs -new=NewNs \
  -p=/path/to/build \
  src/foo.cpp src/bar.cpp src/baz.h
```

- `-p=<dir>`: directory containing `compile_commands.json`.
- `-old=<name>` / `-new=<name>`: unqualified namespace names.
- `-dry-run`: print the planned edits (file, offset, length, replacement
  text) instead of writing to disk

To modify all files in a dir:
```bash
./build/namespace-refactor -old=OldNs -new=NewNs -p=build \
  $(find source include -name '*.cpp' -o -name '*.h')
```