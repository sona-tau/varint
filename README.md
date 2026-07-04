# varint

An unsigned varint C library.

Features:

- minimal, only has: `add`, `sub` & `mul`
- zero dependency: no other libraries were used (except glibc)
- provides dynamic, static and header-only versions of the library

## Implementation Details

This implementation is based off of: [multiformats/unsigned-varint](https://github.com/multiformats/unsigned-varint)

- <https://github.com/multiformats/unsigned-varint>
- <https://en.wikipedia.org/wiki/Variable-length_quantity>

---

This project uses Kelvin versioning (as opposed to the very popular versioning system: semver). This means that once this project reaches 0K, no new releases will come out.

- <https://wiki.xxiivv.com/site/kelvin_versioning.html>

## Building

To compile the project, first compile the build tool:

``` sh
cc nob.c -o nob
```

Then use the build tool to compile the project:

``` sh
./nob build
```

The `./nob` binary is the build tool this project uses.

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


### Developing

This repository uses Nix for reproducibility. If you do not use Nix, you will have to manually install a C compiler (this project uses `gcc`). For development, you will also need:

- bear
- gdb
- universal-ctags
- cppcheck
- doxygen
- clang-tools
- valgrind

To enter the development environment, run:

``` sh
nix develop
```

```sh
# shared objects
nix build .#default.out

# static library and header
nix build .#default.dev

# documentation (HTML and man)
nix build .#default.doc
```

Additionally, this Nix flake uses content-addressed derivations. You might have to enable these in your nix config in order to build:

- <https://nixos.wiki/wiki/Ca-derivations>

You should still be able to open the development environment by removing the `__contentAddresssed = true;` line in [`./flake.nix`](flake.nix).

# Elsewhere

This project is hosted on [tangled.org](https://tangled.org/) and [github.com](https://tangled.org/)

- tangled: <https://tangled.org/stau.space/varint>
- GitHub: <https://github.com/sona-tau/varint>
- Radicle: `rad:z36o1kRdhCBtrnrtBDK1hfRrLMfaj`

This project also provides the following feeds:

- tangled:
  - [all](https://tangled.org/stau.space/varint/feed.atom)
  - [issues](https://tangled.org/stau.space/varint/feed.atom?include=issues)
  - [pull requests](https://tangled.org/stau.space/varint/feed.atom?include=pulls)
  - [commits](https://tangled.org/stau.space/varint/feed.atom?include=commits)
  - [tags](https://tangled.org/stau.space/varint/feed.atom?include=tags)
- GitHub:
  - [releases](https://github.com/sona-tau/varint/releases.atom)
  - [commits](https://github.com/sona-tau/varint/commits.atom)
  - [tags](https://github.com/sona-tau/varint/tags.atom)

# TODO

- add fuzz testing from libFuzzing
- nob should:
``` txt
./nob build {release, tiny, debug, static, dynamic} -m MACHINE

./nob dev {docs, fmt, fmt-check, compile-commands}

./nob pack

./nob clean

./nob test
```
- put docs in `/docs`
  - then use github/tangled actions to create doc pages for this
  - then put the links for the docs in here
- add CA derivation to IPFS
  - then add the CID to this README.md
- make a header-only version
- split `varint.h` into:
  - `varint.h` -> `new`, `free`, `into`
  - `varint-tools.h` -> `to_string`, `from_string`, `add`, `sub`, `mul`

