# 5K

First, the API is almost complete:

- `varint_to_u64`: converts back to a `uint64_t` if possible
- `varint_copy`: copies a varint
- `varint_parse`: parses a byte buffer for a varint

## Variadic arithmetic functions

Arithmetic functions are now variadic macros that check for the following arguments:

- `.l`: the left-hand side of the binary operator
- `.r`: the right-hand side of the binary operator
- `.lenl`: the length of the left-hand side of the binary operator
- `.lenr`: the length of the right-hand side of the binary operator
- `.deallocate`: An enum that determines if any of the arguments to this function call should be freed

This lets you write lines like:

```c
return varint_sub(
	varint_mul(
		varint_new(6), varint_new(10), .deallocate = Both),
	varint_new(10), .deallocate = Both);
```

Instead of:

```c
varint val_6 = varint_new(6);
varint val_7 = varint_new(7);
varint val_10 = varint_new(10);

varint result_42 = varint_mul(val_6, val_7);
free(val_6);
free(val_7);

varint result_32 = varint_sub(result_42, val_10);
free(result_42);
free(val_10);

return result_32;
```

## Improved IO

Instead of having to read and write to a `FILE*` now you can just convert back and forth from strings as big integers:

```c
char *varint_to_string(varint a);
varint varint_from_string(const char *s, size_t *consumed);
```

# 272K

Added `varint_into`: Encode a `uint64_t` into a varint, writing it to `out`. _If the amount of bytes that were allocated weren't enough to encode `a` the `size_t` returned is 0._
- *param `out`*: the varint to encode into
- *param `nbytes`*: the amount of bytes that were allocated for out
- *param `a`*: the `uint64_t` to encode
- *return*: The amount of bytes that were written.
```c
size_t varint_into(varint *out, size_t nbytes, uint64_t a);
```

Changed the API for the following:
```c
varint varint_add(varint l, size_t lenl, varint r, size_t lenr);
varint varint_addn(varint l, varint r);

varint varint_sub(varint l, size_t lenl, varint r, size_t lenr);
varint varint_subn(varint l, varint r);

varint varint_mul(varint l, size_t lenl, varint r, size_t lenr);
varint varint_muln(varint l, varint r);
```

Added `varint_write`: Writes a varint to a file descriptor. Writes to `fd` a C string representing `a` in base 10.
- *param `fd`*: the file descriptor to write to
- *param `a`*: a well-formed varint
```c
void varint_write(FILE *f, varint a);
```

Added `varint_read`: Reads a varint from a file descriptor.
* Reads from `fd` assuming `fd` is a C string (null-terminated) representing the number in base 10.
* *param `fd`*: the file descriptor to read from
* *return*: the varint that was read
```c
varint varint_read(FILE *f);
```

Removed `varint_print`.

# 373K

Added the following functions:

```c
/*! Convert a uint64_t into a varint.
 */
varint varint_new(uint64_t a);

/*! Calculates the length of a varint by checking how many bytes it uses.
 */
size_t varint_length(varint a);

/*! Frees a varint by a pointer to the varint.
 * This is a helper function for vint.
 */
inline static void varint_free(void *p) { free(*(void **)p); }

/*! Create a varint whose memory gets free'd when the variable goes out of
 * scope.
 */
#define vint __attribute__((cleanup(varint_free))) varint

/*! Add two varints.
 */
varint varint_add(varint l, varint r);

/*! Subtract two varints.
 * Precondition: l >= r
 */
varint varint_sub(varint l, varint r);

/*! Multiply two varints.
 */
varint varint_mul(varint l, varint r);

/*! Divide a varint by 10 returning the quotient and remainder.
 * Caller must free the quotient.
 */
divmod10 varint_div10(varint a);

/*! Compares first argument with second argument. When the first is greater than
 * the second, returns 1, when equal returns 0, when less returns -1.
 */
int varint_cmp(varint a, varint b);

/*! Macro to compare less than.
 */
#define varint_lt(a, b) (varint_cmp((a), (b)) < 0)

/*! Macro to compare greatr than.
 */
#define varint_gt(a, b) (varint_cmp((a), (b)) > 0)

/*! Macro to compare for equality.
 */
#define varint_eq(a, b) (varint_cmp((a), (b)) == 0)

/*! Prints a varint to stdout.
 */
void varint_print(varint a);

/*! Prints each byte a varint uses in hexadecimal.
 */
void varint_print_debug(varint a);
```
