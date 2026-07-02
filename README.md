# varint

Bootstrap once:

```sh
cc nob.c -o nob
```

Then:

```sh
./nob          # build static + dynamic
./nob test     # run tests
./nob static   # build .build/libvarint.a
./nob dynamic  # build .build/libvarint.so
```

The `nob` binary will recompile itself automatically if `nob.c` changes.
