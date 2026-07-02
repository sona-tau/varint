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
