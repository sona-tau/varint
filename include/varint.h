#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file varint.h
 * \brief Unsigned variable-length integer encoding.
 *
 * This library implements the unsigned unsigned-varint encoding used by
 * Multiformats. See: https://github.com/multiformats/unsigned-varint
 *
 * A varint is a heap-allocated, byte sequence represented as `uint8_t *`.
 *
 * \note Every function that returns a `varint` (or a `divmod10` containing one)
 * transfers ownership to the caller. The caller must release that memory with
 * `free()` or by declaring the variable with the `vint` cleanup macro.
 *
 * The library does not use any global mutable state and is safe to call from
 * multiple threads, provided the C memory allocator is thread-safe.
 *
 * All functions assume the input varints are well-formed (valid continuation
 * bits). Passing malformed varints results in undefined behavior.
 *
 * A valid continuation bit is the MSB of a uint8_t and it is:
 * - 0 when the next byte does not belong to the current varint
 * - 1 when the next byte does belong to the current varint
 *
 * Therefore, this library assumes that when the MSB of a uint8_t is:
 * - 0:
 *   - the next byte cannot be read
 * - 1:
 *   - the next byte can be read
 */

/* ---- CORE ---- */

/*! An unsigned varint; a more restricted version of LEB128.
 */
typedef uint8_t *varint;

/*! Result of dividing a varint by 10.
 */
typedef struct {
  varint q;  /*!< Quotient; caller must free! */
  uint8_t r; /*!< Remainder; in [0, 9]. */
} divmod10;

typedef enum {
  Neither = 0, /*!< 00 */
  Right = 1,   /*!< 01 */
  Left = 2,    /*!< 10 */
  Both = 3     /*!< 11 */
} Deallocate;

/*! Encode a `uint64_t` into a varint.
 * \param a the `uint64_t` to encode
 * \warning If not able to be allocated, the function exits with \a
 * `EXIT_FAILURE`.
 * \return A newly allocated varint. The caller owns the returned value.
 */
varint varint_new(uint64_t a);

/*! Encode a `uint64_t` into a varint, writing it to `out`.
 * \param[out] out the varint to encode into
 * \param nbytes the amount of bytes that were allocated for out
 * \param a the `uint64_t` to encode
 * \warning If the amount of bytes that were allocated weren't enough to encode
 * `a` the `size_t` returned is 0.
 * \return The amount of bytes that were written.
 */
size_t varint_into(varint *out, size_t nbytes, uint64_t a);

/*! Calculates the number of bytes in a varint.
 * It is useful to calculate the length of a varint if needed, but this is an
 * O(n) operation, so do this when you have to and don't do it often.
 * \param a a well-formed varint.
 * \return The number of bytes in a varint.
 */
size_t varint_length(varint a);

/*! Frees a varint pointed to by a pointer.
 * \param[in] p the pointer to the varint
 * \warning This function is an implementation detail for the `vint` cleanup
 * macro. Do not call it directly.
 */
inline static void varint_free(void *p) { free(*(void **)p); }

/*! Declare a varint that is automatically freed when it goes out of scope.
 * \warning This is a GCC/Clang extension (`__attribute__((cleanup(...)))`). It
 * has no effect on MSVC or other compilers.
 *
 * Example:
 * \code
 *     varint a = varint_new(1234);
 *     varint b = varint_new(5678);
 *     free(a); // free a
 *     free(b); // free b
 * \endcode
 * Example:
 * \code
 * {
 *     vint a = varint_new(1234);
 *     vint b = varint_new(5678);
 * } // a and b are freed after this point
 * \endcode
 *
 * \warning Since this kind of declaration will automatically free the varint
 * when the variable comes out of scope, the user must not free this memory.
 */
#define vint __attribute__((cleanup(varint_free))) varint

/* ---- ARITHMETIC ---- */

typedef struct {
  varint l;
  size_t lenl;
  varint r;
  size_t lenr;
  Deallocate deallocate;
} binop_args;

/*! Add two varints.
 * \note This is a variadic function of which you need to specify at least two
 * arguments
 * \param l a well-formed varint
 * \param r a well-formed varint
 * \note Caller owns the returned varint.
 * \return the result of adding `l` and `r` (`l + r`)
 * \code
 * \endcode
 */
#define varint_add(...) var_varint_add((binop_args){__VA_ARGS__})

varint _varint_add(varint l, size_t lenl, varint r, size_t lenr,
                   Deallocate deallocate);

static inline varint var_varint_add(binop_args args) {
  args.lenl = args.lenl == 0 ? varint_length(args.l) : args.lenl;
  args.lenr = args.lenr == 0 ? varint_length(args.r) : args.lenr;
  args.deallocate = args.deallocate > Both ? 0 : args.deallocate;
  return _varint_add(args.l, args.lenl, args.r, args.lenr, args.deallocate);
}

/*! Subtract two varints.
 * \param l a well-formed varint
 * \param r a well-formed varint
 * \pre `l >= r` (use `varint_cmp` / `varint_lt` to check).
 * \note Caller owns the returned varint.
 * \return the result of subtracting `r` from `l` (`l - r`)
 */
varint _varint_sub(varint l, size_t lenl, varint r, size_t lenr,
                   Deallocate deallocate);

static inline varint var_varint_sub(binop_args args) {
  args.lenl = args.lenl == 0 ? varint_length(args.l) : args.lenl;
  args.lenr = args.lenr == 0 ? varint_length(args.r) : args.lenr;
  args.deallocate = args.deallocate > Both ? 0 : args.deallocate;
  return _varint_sub(args.l, args.lenl, args.r, args.lenr, args.deallocate);
}

#define varint_sub(...) var_varint_sub((binop_args){__VA_ARGS__})

/*! Multiply two varints.
 * \param l a well-formed varint
 * \param r a well-formed varint
 * \note Caller owns the returned varint.
 * \return the result of multiplying `l` and `r` (`l * r`)
 */
varint _varint_mul(varint l, size_t lenl, varint r, size_t lenr,
                   Deallocate deallocate);

static inline varint var_varint_mul(binop_args args) {
  args.lenl = args.lenl == 0 ? varint_length(args.l) : args.lenl;
  args.lenr = args.lenr == 0 ? varint_length(args.r) : args.lenr;
  args.deallocate = args.deallocate > Both ? 0 : args.deallocate;
  return _varint_mul(args.l, args.lenl, args.r, args.lenr, args.deallocate);
}

#define varint_mul(...) var_varint_mul((binop_args){__VA_ARGS__})

/*! Divide a varint by 10.
 * \param a a well-formed varint
 * \note Caller owns `divmod10.q`.
 * \return Quotient and remainder.
 */
divmod10 varint_div10(varint a);

/*! Compare two varints.
 * \param a a well-formed varint
 * \param b a well-formed varint
 * \return `1` if `a > b`, `0` if `a == b`, `-1` if `a < b`
 */
int varint_cmp(varint a, varint b);

/*! True if `a < b`. */
#define varint_lt(a, b) (varint_cmp((a), (b)) < 0)

/*! True if `a > b`. */
#define varint_gt(a, b) (varint_cmp((a), (b)) > 0)

/*! True if `a == b`. */
#define varint_eq(a, b) (varint_cmp((a), (b)) == 0)

/* ---- STRING ---- */

/*! Writes a varint to a file descriptor.
 *  Writes to `fd` a C string representing `a` in base 10.
 *  \param fd the file descriptor to write to
 *  \param a a well-formed varint
 */
char *varint_to_string(varint a);

/*! Reads a varint from a file descriptor.
 * Reads from `fd` assuming `fd` is a C string (null-terminated) representing
 * the number in base 10.
 * \param fd the file descriptor to read from
 * \return the varint that was read
 */
varint varint_from_string(const char *s);

/* ---- DEBUG ---- */

/*! Prints each byte in hexadecimal to stdin.
 */
void varint_print_debug(varint a);

#ifdef __cplusplus
}
#endif
