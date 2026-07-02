# varint

An unsigned varint C library.

Features:

- minimal, only has:
  - arithmetic: `add`, `sub` & `mul`
  - IO: `show`, `read`
  - other: `div10`
- zero dependency: no other libraries were used
- provides dynamic, static and header-only versions of the library

## Implementation Details

This implementation is based off of: [multiformats/unsigned-varint](https://github.com/multiformats/unsigned-varint)

- <https://github.com/multiformats/unsigned-varint>
- <https://en.wikipedia.org/wiki/Variable-length_quantity>

This repository uses Nix for reproducibility. If you do not use Nix, you will have to manually install:

- gcc
- bear
- gdb
- universal-ctags
- cppcheck
- doxygen
- clang-tools
- valgrind

Additionally, this Nix flake uses content-addressed derivations. You might have to enable these in your nix config:

- <https://nixos.wiki/wiki/Ca-derivations>

## Building


To compile the whole project, run:

```sh
# shared objects
nix develop .#default.out

# static library and header
nix develop .#default.dev

# documentation (HTML and man)
nix develop .#default.doc
```

## Developing

First, get inside the development environment (dev-env) by running:

``` sh
nix develop
```

### Inside dev-env

Bootstrap once:

```sh
cc nob.c -o nob
```

Then:

```sh
./nob                 # build static + dynamic (default)
./nob build-release   # same as above: heavy speed optimizations and no debug info or asserts
./nob build-tiny      # same as above: heavy size optimizations and no debug info or asserts
./nob build-debug     # same as above: absolutely no optimizations and lots of debug info

./nob test     # run tests
./nob static   # build .build/libvarint.a
./nob dynamic  # build .build/libvarint.so

./nob help  # Show usage
```

The `nob` binary will recompile itself automatically if `nob.c` changes.

# TODO

- output binaries for:
  - `-Ofast -s -DNDEBUG -march=native -flto` for speed
  - `-Oz -s -DNDEBUG -ffunction-sections -fdata-sections -Wl,--gc-sections -fno-ident -fno-asynchronous-unwind-tables` for size
  - `-O0 -g3 -ggdb3 -glldb -gdwarf-4 -Wall -Wextra -DDEBUG` for debug 
- add tests for add, sub, mul, div
- IO:
  - add `varint_read` (reads from file)
  - rename `varint_print` to `varint_show` and make it return a `const char*` to a null-terminated string (caller must free)
- add fuzz testing from libFuzzing
