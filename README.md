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

This project uses Kelvin versioning (as opposed to the very popular versioning system: semver). This means that once this project reaches 0K, no new releases will come out.

- <https://wiki.xxiivv.com/site/kelvin_versioning.html>

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

The `./nob` binary is the build tool this project uses.

Then:

```txt
Usage: ./nob [OPTIONS]

Options:
  -b, build,            --build               Build: static and dynamic libraries (same as debug)
      release,          --release             Build: speed opts & no debug info & no assertions
      tiny,             --tiny                Build: size opts & no debug & no assertions
      debug,            --debug               Build: debug info & assertions
      static,           --static              Build: .build/libvarint.a
      dynamic,          --dynamic             Build: .build/libvarint.so
  -t, test,             --test                Test: tests/test_*.c
      valgrind,         --valgrind            Valgrind on tests: no sanitizers
      strace,           --strace              STrace on tests: no sanitizers
      cppcheck,         --cppcheck            cppcheck static analysis
      ctags,            --ctags               Generate tags file for editor navigation
      fmt,              --fmt                 clang-format in-place
      fmt-check,        --fmt-check           Check formatting (for CI)
      compile-commands, --compile-commands    Generate compile_commands.json via bear
      docs,             --docs                Generate HTML & man docs in .build/docs/
      pack,             --pack [ver] [pfx]    Pack library as tar.gz (semver optional)
      clean,            --clean               Remove .build/
  -h, help,             --help                Print help
```

The `./nob` binary will recompile itself automatically if `nob.c` changes.

# TODO

- add tests for add, sub, mul, div
- IO:
  - add `varint_read` (reads from file)
  - rename `varint_print` to `varint_show` and make it return a `const char*` to a null-terminated string (caller must free)
- add fuzz testing from libFuzzing
- change semver code in nob.c to kelvin versioning
