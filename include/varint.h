#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t *varint;
typedef struct {
	varint q;
	uint8_t r;
} divmod10;

varint varint_new(uint64_t a);
size_t varint_length(varint a);
inline static void varint_free(void *p) { free(*(void **)p); }
#define vint __attribute__((cleanup(varint_free))) varint

varint varint_add(varint l, varint r);
varint varint_sub(varint l, varint r);
varint varint_mul(varint l, varint r);

divmod10 varint_div10(varint a);

int varint_cmp(varint a, varint b);
#define varint_lt(a, b) (varint_cmp((a), (b)) < 0)
#define varint_gt(a, b) (varint_cmp((a), (b)) > 0)
#define varint_eq(a, b) (varint_cmp((a), (b)) == 0)

void varint_print(varint a);
void varint_print_debug(varint a);

